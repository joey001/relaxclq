#if defined(_OPENMP) || defined(OPENMP) || defined(CILK) || defined(CILKP) || defined(PARLAY_OPENMP) || defined(PARLAY_CILKPLUS) || defined(PARLAY_OPENCILK) || defined(PARLAY_TBB)
#error "P004 requires the pinned native Parlay scheduler"
#endif
int k,lb,bd;
double time_limit=-1;
#include <parlay/parallel.h>
#include "listPlex.h"
#undef parallel_for
#include <chrono>
#include <cmath>
#include <cstdio>
#include <queue>
#include <atomic>
std::mutex coutMutex;
namespace {
using Clock = std::chrono::steady_clock;
Clock::time_point searchStart;
}
namespace ListPlex
{
    void checkTimeLimit() {
        if (time_limit >= 0 && std::chrono::duration<double>(Clock::now()-searchStart).count() >= time_limit) {
            std::lock_guard<std::mutex> lock(coutMutex);
            fprintf(stderr,"Time limit reached (incomplete)\n");
            std::exit(1);
        }
    }

    struct PlexEmitor {
        unsigned long long count=0;
#ifdef LISTPLEX_EMIT_SOLUTIONS
        const std::vector<int>& blk;
        const std::vector<int>& originalIDs;
        PlexEmitor(const std::vector<int>& b,const std::vector<int>& ids):blk(b),originalIDs(ids) {}
#else
        PlexEmitor(const std::vector<int>&,const std::vector<int>&) {}
#endif
        void emitPlex(const int* list,int size) {
            ++count;
#ifdef LISTPLEX_EMIT_SOLUTIONS
            std::vector<int> solution;
            solution.reserve(size);
            for (int i=0;i<size;++i) solution.push_back(originalIDs[blk[list[i]]]);
            std::sort(solution.begin(),solution.end());
            std::lock_guard<std::mutex> lock(coutMutex);
            printf("SOLUTION");
            for (int v:solution) printf(" %d",v);
            printf("\n");
#endif
        }
    };

    struct DecomposeMaxChecker
    {
        const graph<int> subg; //check in the subgraph(contains all possible nodes and edges) for cache-efficiency

        RandSet<int> uni;
        DecomposeMaxChecker(const graph<int> &_subg, int leftSize)
            : subg(_subg), uni(leftSize){}

        //Count the number of adjacent vertices of u in the plex
        bool canGlobalAdd(int u, int szplex,int* plex){
            int cnt = 0;
            for (int j = 0; j < szplex;++j)
            {
                const int ele = plex[j];
                if(binary_search(subg.V[ele].NeighborsLeft,subg.V[ele].NeighborsLeft+subg.V[ele].degreeLeft,u))
                    cnt++;
            }
            if (cnt + k >= szplex + 1)
                return true; //can add
            return false;	//can not add
        }

        //Check if the vertices in list in a globaly maximal k-plex
        //i.e., maximality : all vertices in Preds cannot form a larger k-plex with list.
        bool isMaximal(int *list, int szplex, ShiftedCounts neiInP)
        {
#ifdef P011_DIAGNOSTICS
            for(int i=0;i<szplex;++i){
                int v=list[i],count=0;
                for(int j=0;j<szplex;++j)
                    count+=std::binary_search(subg.V[v].Neighbors,subg.V[v].Neighbors+subg.V[v].degree,list[j]);
                if(neiInP[v]!=count) std::abort();
            }
#endif
            // saturated vertices;
            std::vector<int> sat(szplex); //saturated vertices in plex
            int nsat = 0;
            bool max = true;
            for (int i = 0; i < szplex; i++){
                const int v = list[i];
                if (neiInP[v] + k == szplex){
                    sat[nsat++] = v;
                }
            }
            if (nsat){
                int v0 = sat[0];
                int *commons = new int[subg.V[v0].degreeLeft];
                copy(subg.V[v0].NeighborsLeft, subg.V[v0].NeighborsLeft + subg.V[v0].degreeLeft, commons);
                int szcom = subg.V[v0].degreeLeft;
                for (int i = 1; i < nsat; i++){
                    int u = sat[i];
                    szcom = utils::interSection(subg.V[u].NeighborsLeft, subg.V[u].degreeLeft, commons, szcom, commons);
                }
                //(commons,szcom) contains all common left-adajcent vertices of saturated vertices
                if(nsat==1){
                    for (int i = 0; i < k;++i){
                        int u = list[i];
                        for (int j = 0; j < subg.V[u].degreeLeft;++j){
                            const int ele = subg.V[u].NeighborsLeft[j];
                            if(!uni.contains(ele))uni.add(ele);
                        }
                    }
                    int sz = szcom;
                    szcom = 0;
                    for (int i = 0; i < sz;++i)
                    {
                        if(uni.contains(commons[i]))
                            commons[szcom++]=commons[i];
                    }
                    uni.clearByEle();
                }
                for (int i = 0; i < szcom; i++){
                    int u = commons[i];
                    if (canGlobalAdd(u, szplex, list)){ // not maximal because we can add u to the solution
                        max = false;
                        break;
                    }
                }
                delete[] commons;
            }
            else{//no saturated vertex
                for (int i = 0; i < k;++i){
                    int u = list[i];
                    for (int j = 0; j < subg.V[u].degreeLeft;++j){
                        const int ele = subg.V[u].NeighborsLeft[j];
                        if(!uni.contains(ele))uni.add(ele);
                    }
                }
                for (int i = 0; i < uni.sz;++i)
                {
                    const int ele = uni.members[i];
                    //local->global->degeneracy->status
                    if (canGlobalAdd(ele, szplex,list)){
                        max = false;
                        break;
                    }
                }
                uni.clearByEle();
	    	}
	    	return max;
		}
    };

    //Decompose a local graph contains vstart and its 1a2hop-neighbors from the global graph
    int decompose(const Graph &g,const int vstart,vector<int> &neibors_1a2hop,vector<int> &neibors_1a2hopLeft,const int* dpos,vector<uint8_t> &visited,vector<int> &count,vector<int> &neibors,vector<int> &neiborsLeft){
        //collect 1 hop neigbors
        visited[vstart]  = 1;
        for(int j = 0; j < g.V[vstart].degree; j++){
            const int nei = g.V[vstart].Neighbors[j];
            if (!visited[nei]){
                visited[nei] = 1;
                if(dpos[vstart]<dpos[nei])//1-hop right-neighbor
                    neibors_1a2hop.emplace_back(nei);
                else//1-hop left-neighbor
                    neibors_1a2hopLeft.emplace_back(nei);
            }
        }
        auto filter = [&](const std::vector<int>& origin,const std::vector<int>& neibors_1hop,const int thres) {
            std::vector<int> filtered;
            const int sz2 = neibors_1hop.size();
            for (const auto u : origin) {
                size_t cnt = 0;
                const int sz1 = g.V[u].degree;
                //choose proper algorithm by time complexity estimation
                if(sz2 * log2(sz1) < (sz2 + sz1)){
                    for (auto v : neibors_1hop) {
                        if (g.isAdj(u,v))cnt++;
                    }
                }
                else{
                    const int *const lst1 = g.V[u].Neighbors;
                    const int *const lst2 = neibors_1hop.data();
                    int j1 = 0, j2 = 0;
                    while (j1 < sz1 && j2 < sz2) {
                        if (lst1[j1] < lst2[j2])
                            ++j1;
                        else if (lst1[j1] > lst2[j2])++j2;
                        else {
                            cnt++,j1++, j2++;
                        }
                    }
                }
                if (thres <= 0 || cnt >= size_t(thres)) filtered.push_back(u);
                else
                    visited[u] = 0;
            }
            return filtered;
        };
        size_t sz;
        const int lb_2k = lb - 2 * k;
        do {
            sz = neibors_1a2hop.size();
            if(sz<bd)goto CLEAN;
            //the number of common neighbors with vstart >= lb-2*k
            neibors_1a2hop = filter(neibors_1a2hop, neibors_1a2hop, lb_2k);
        } while (neibors_1a2hop.size() < sz);
        //the number of common neighbors with vstart >= lb+1-2*k
        neibors_1a2hopLeft = filter(neibors_1a2hopLeft, neibors_1a2hop,lb_2k+1);

        //collect 2-hop neigbors
        for(const auto nei1 : neibors_1a2hop){
            for (int j = 0; j < g.V[nei1].degree; j++)
            {
                const int nei2 = g.V[nei1].Neighbors[j];
                if (!visited[nei2]){
                    if(!count[nei2]){
                        if(dpos[vstart]<dpos[nei2])
                            neibors.emplace_back(nei2);
                        else
                            neiborsLeft.emplace_back(nei2);
                    }
                    count[nei2]++;
                }
            }
        }
        neibors_1a2hop.emplace_back(vstart);
        swap(neibors_1a2hop[0], neibors_1a2hop[neibors_1a2hop.size() - 1]);
        //In fact, neibors_1a2hop contains the vstart at the first

        //restore the global variables
    CLEAN:
        for (auto v : neibors_1a2hop ){
            visited[v] = 0;
        }
        for (auto v : neibors_1a2hopLeft){
            visited[v] = 0;
        }
        for (int v : neibors) {
            //the number of common neighbors with vstart >= lb-2*k+2
            if (count[v] >= lb_2k + 2) {
                neibors_1a2hop.emplace_back(v);
            }
            count[v] = 0;
        }
        for (int v : neiborsLeft) {
            //the number of common neighbors with vstart >= lb+1-2*k+2
            if (count[v] >= lb_2k+3) {
                neibors_1a2hopLeft.emplace_back(v);
            }
            count[v] = 0;
        }
        neibors.clear();
        neiborsLeft.clear();
        visited[vstart] = 0; // no residual state may cross root boundaries
        return sz+1;
    }

    //Build edges among the local nodes
    Graph buildSubgraph(const Graph &g,const vector<int>& blk,const vector<int>& left,vector<uint8_t> &inBlk, vector<uint8_t> &inLeft,const int hopSz,vector<uint8_t> &isNei,vector<uint8_t> &commonMtx,UpdateRows& updateRows){
        const int blkSz = blk.size();
        const int leftSz = left.size();
        vertex<int> *vertices = newA(vertex<int>, blkSz);
        int edgesHop=0,edges=0;
        for (int i = 0; i < blkSz; i++){
            const int origin = blk[i];
            int ne = 0, neLeft = 0;
            for (int j = 0; j < g.V[origin].degree; j++){
                const int nei = g.V[origin].Neighbors[j];
                if (inBlk[nei]){ // in block
                    ne++;
                    isNei[nei]=true;
                }
                if (inLeft[nei]){
                    neLeft++;
                    isNei[nei]=true;
                }
            }
            vertices[i].Neighbors = newA(int,ne);
            vertices[i].NeighborsLeft = newA(int,neLeft);
            vertices[i].degree=ne;
            vertices[i].degreeLeft=neLeft;

            // Fill selected closed complements in the existing matrix traversal.
            int* closed=updateRows.initializeRow(i,ne,vertices[i].Neighbors);
            int closedCnt=0;
            //add right edges
            int cnt = 0;
            for (int j = 0; j < blkSz; j++){
                const int nei = blk[j];
                commonMtx[size_t(i)*blkSz+j]=isNei[nei];
                if (isNei[nei]){
                    vertices[i].Neighbors[cnt++] = j;
                    isNei[nei]=false;
                }
                else if(closed) closed[closedCnt++]=j; // includes i (no self loops)
                if(j==hopSz-1)vertices[i].degreeHop=cnt;
            }
            assert(!closed || closedCnt==blkSz-ne);
            //add left edges
            cnt = 0;
            for (int j = 0; j < leftSz; j++){
                const int nei = left[j];
                if (isNei[nei]){
                    vertices[i].NeighborsLeft[cnt++] = j;
                    isNei[nei]=false;
                }
            }
            if(i<hopSz)edgesHop+=vertices[i].degreeHop;
            edges+=vertices[i].degree;
        }
        const bool proper=(double(edgesHop)/(hopSz*(hopSz-1)))<0.9 && (double(edges)/(blkSz*(blkSz-1)))<0.85;
        return Graph(vertices, blkSz, 0, nullptr, proper);
    }

    Graph peelGraph(const Graph &g,bool* const mark,int * const resNei,vector<int>& originalIDs){
        const int n=g.n;
        {
            queue<int> Q;
            for(int i=0;i<n;++i){
                if(g.V[i].degree<bd){
                    mark[i]=false;
                    Q.push(i);
                }
                else{
                    mark[i]=true;
                    resNei[i]=g.V[i].degree;
                }
            }
            while(Q.size()){
                const int ele=Q.front();
                Q.pop();
                for(int i=0;i<g.V[ele].degree;++i){
                   const int nei=g.V[ele].Neighbors[i];
                    if(mark[nei]){
                        const int old=resNei[nei]--;
                        if(old==bd){
                            mark[nei]=false;
                            Q.push(nei);
                        }
                    }
                }
            }
        }

        int* const map=new int[n];
        for(int i=0;i<n;++i)map[i]=i;
        _seq<int> leadList = sequence::pack(map, mark, n);

        const int pn=leadList.n;
        originalIDs.assign(leadList.A,leadList.A+pn);
        for(int i=0;i<pn;++i)map[leadList.A[i]]=i;
        vertex<int> *vertices = newA(vertex<int>, pn);
        for (int i = 0; i < pn; i++){
            const int ori = leadList.A[i];
            const int en=resNei[ori];
            vertices[i].Neighbors = newA(int,en);
            vertices[i].NeighborsLeft = nullptr;
            vertices[i].degree=en;
            int cursor=0;
            for(int j=0;j<g.V[ori].degree;++j){
                const int nei=g.V[ori].Neighbors[j];
                if(mark[nei])vertices[i].Neighbors[cursor++]=map[nei];
            }
        }
        delete[] map;
        leadList.del();
        return Graph(vertices, pn, 0, nullptr);
    }

    void buildCommonMtx(const Graph &subg,const int blkSz,const int hopSz,vector<uint8_t> &commonMtx){
        const int thresPP1=lb-2*k-max(k-3,0),thresPP2=lb-2*k+2-max(k-3,0);
        const int thresPC1=lb-2*k-max(k-2,0),thresPC2=lb-2*k+2-max(k-2,0);
        const int thresCC1=lb-2*k-(k-1),thresCC2=lb-2*k+2-(k-1);
        for(int i=0;i<hopSz;++i){
            for(int j=0;j<i;++j){
                const int common=utils::commonEle(
                    subg.V[i].Neighbors,
                    subg.V[i].degreeHop,
                    subg.V[j].Neighbors,
                    subg.V[j].degreeHop
                );
                if(commonMtx[size_t(i)*blkSz+j]){
                    if(common>thresCC1)commonMtx[size_t(i)*blkSz+j]=LINK2MORE;
                    else if(common==thresCC1)commonMtx[size_t(i)*blkSz+j]=LINK2EQUAL;
                    else commonMtx[size_t(i)*blkSz+j]=LINK2LESS;
                }
                else{
                    if(common>thresCC2)commonMtx[size_t(i)*blkSz+j]=UNLINK2MORE;
                    else if(common==thresCC2)commonMtx[size_t(i)*blkSz+j]=UNLINK2EQUAL;
                    else commonMtx[size_t(i)*blkSz+j]=UNLINK2LESS;
                }
                commonMtx[size_t(j)*blkSz+i]=commonMtx[size_t(i)*blkSz+j];
            }
        }
        for(int i=hopSz;i<blkSz;++i){
            for(int j=0;j<hopSz;++j){
                const int common=utils::commonEle(
                    subg.V[i].Neighbors,
                    subg.V[i].degreeHop,
                    subg.V[j].Neighbors,
                    subg.V[j].degreeHop
                );
                if(commonMtx[size_t(i)*blkSz+j]){
                    if(common>thresPC1)commonMtx[size_t(i)*blkSz+j]=LINK2MORE;
                    else if(common==thresPC1)commonMtx[size_t(i)*blkSz+j]=LINK2EQUAL;
                    else commonMtx[size_t(i)*blkSz+j]=LINK2LESS;
                }
                else{
                    if(common>thresPC2)commonMtx[size_t(i)*blkSz+j]=UNLINK2MORE;
                    else if(common==thresPC2)commonMtx[size_t(i)*blkSz+j]=UNLINK2EQUAL;
                    else commonMtx[size_t(i)*blkSz+j]=UNLINK2LESS;
                }
                commonMtx[size_t(j)*blkSz+i]=commonMtx[size_t(i)*blkSz+j];
            }
            if(k==2)continue;
            for(int j=hopSz;j<i;++j){
                const int common=utils::commonEle(
                    subg.V[i].Neighbors,
                    subg.V[i].degreeHop,
                    subg.V[j].Neighbors,
                    subg.V[j].degreeHop
                );
                if(commonMtx[size_t(i)*blkSz+j]){
                    if(common>thresPP1)commonMtx[size_t(i)*blkSz+j]=LINK2MORE;
                    else if(common==thresPP1)commonMtx[size_t(i)*blkSz+j]=LINK2EQUAL;
                    else commonMtx[size_t(i)*blkSz+j]=LINK2LESS;
                }
                else{
                    if(common>thresPP2)commonMtx[size_t(i)*blkSz+j]=UNLINK2MORE;
                    else if(common==thresPP2)commonMtx[size_t(i)*blkSz+j]=UNLINK2EQUAL;
                    else commonMtx[size_t(i)*blkSz+j]=UNLINK2LESS;
                }
                commonMtx[size_t(j)*blkSz+i]=commonMtx[size_t(i)*blkSz+j];
            }
        }
    }

    // P004: serial preprocessing, then reusable logical root streams.
    // Root callbacks never yield or create nested parallel work. Every mutable
    // search object is callback-owned; shared graph/order vectors are read-only.
    void decomposableSearch(const Graph &g) {
        searchStart=Clock::now();
        std::vector<int> dpos(g.n,INT_MAX),dseq(g.n),resNei(g.n),originalIDs;
        std::unique_ptr<bool[]> mark(new bool[g.n]);
        Graph peelG=peelGraph(g,mark.get(),resNei.data(),originalIDs);
        const int pn=peelG.n;
        if (pn) {
            ListLinearHeap<int> heap(pn,pn-1);
            heap.init(pn,pn-1);
            for (int i=0;i<pn;++i) heap.insert(i,peelG.V[i].degree);
            for (int i=0;i<pn;++i) {
                int u,key; heap.pop_min(u,key); dpos[u]=i; dseq[i]=u;
                for (int j=0;j<peelG.V[u].degree;++j) {
                    const int nei=peelG.V[u].Neighbors[j];
                    if (dpos[nei]==INT_MAX) heap.decrement(nei);
                }
            }
        }
        const auto rootsStart=Clock::now();
        const size_t workers=parlay::num_workers();
        printf("Parlay workers: %zu\n",workers);
        // Q*n <= n + sum_v degree(v)^2. Saturate at the useful worker cap
        // before adding, so even enormous input bounds cannot overflow.
        const size_t cap=std::min(workers,size_t(pn));
        const uint64_t budget=cap ? uint64_t(cap-1)*uint64_t(pn) : 0;
        uint64_t degreeWork=0;
        for (int v=0;v<pn && degreeWork<budget;++v) {
            const uint64_t d=peelG.V[v].degree;
            degreeWork+=std::min(d*d,budget-degreeWork);
        }
        const size_t streams=pn ? std::min(cap,size_t(1+degreeWork/uint64_t(pn))) : 0;
        struct Result { unsigned long long count=0; size_t valid=0,roots=0; unsigned long long snapshots=0,snapshotCapacityBytes=0;
#ifdef P011_DIAGNOSTICS
            MinorityStats stats;
#endif
        };
        std::vector<Result> results(streams);
        std::atomic<size_t> next{0};
        parlay::parallel_for(0,streams,[&](size_t stream) {
            // Q callback identities own scratch. Each current root strictly joins
            // every descendant before this stream reuses any root buffer.
            vector<uint8_t> visited(pn,0),inBlk(pn,0),inLeft(pn,0),commonMtx;
            vector<int> count(pn,0),neibors,neiborsLeft,blk,left,neiInP,neiInG;
            Stack<int> plex,exclStack;
            VtxSet cand1,cand2,excl,exclBK;
            Result result; // publish once; adjacent result records never false-share in the root loop
            for (;;) {
                const size_t idx=next.fetch_add(1,std::memory_order_relaxed);
                if (idx>=size_t(pn)) break;
                ++result.roots;
                checkTimeLimit();
                blk.clear();left.clear();
                const int vstart=dseq[idx];
                const int hopSz=decompose(peelG,vstart,blk,left,dpos.data(),visited,count,neibors,neiborsLeft);
                const int blkSz=blk.size();
                if (blkSz<lb) continue;
                ++result.valid;
                const int hop2Sz=blkSz-hopSz,leftSz=left.size();
                for (int u:blk) inBlk[u]=true;
                for (int u:left) inLeft[u]=true;
                commonMtx.resize(size_t(blkSz)*blkSz);
                neiInP.assign(blkSz,0);neiInG.resize(blkSz);
                plex.resize(blkSz);exclStack.resize(blkSz);
                cand1.resize(blkSz);cand2.resize(blkSz);excl.resize(blkSz);exclBK.resize(blkSz);
                UpdateRows updateRows(blkSz); // finalized here; lives through all joins/cleanup
                Graph subg=buildSubgraph(peelG,blk,left,inBlk,inLeft,hopSz,visited,commonMtx,updateRows);
                if (subg.proper) buildCommonMtx(subg,blkSz,hopSz,commonMtx);
                for (int j=0;j<subg.n;++j) {
                    neiInG[j]=subg.V[j].degreeHop;
                    if (j<hopSz) cand1.add(j); else cand2.add(j);
                }
                DecomposeMaxChecker maxchecker(subg,leftSz);
                PlexEmitor emitor(blk,originalIDs);
                KplexListor<DecomposeMaxChecker,PlexEmitor> listor(
                    subg,&maxchecker,&emitor,&plex,&cand1,&cand2,&excl,&exclBK,&exclStack,
                    {neiInP.data(),0},{neiInG.data(),0},hopSz,hop2Sz,commonMtx,updateRows);
                std::atomic<int> forkCredits(blkSz-1);
                listor.forkCredits=workers>1 ? &forkCredits : nullptr;
                listor.cand1ToPlex(0)->kSearch(k-1);
                const unsigned long long snapshots=blkSz-1-forkCredits.load(std::memory_order_relaxed);
                result.snapshots+=snapshots;
                result.snapshotCapacityBytes+=snapshots*(48ULL*blkSz+8ULL*leftSz+
                    16ULL*(blkSz/32+1)+4ULL*(leftSz/32+1));
                result.count+=emitor.count;
                for (int u:blk) inBlk[u]=false;
                for (int u:left) inLeft[u]=false;
                listor.plexToCand1();
#ifdef P011_DIAGNOSTICS
                if(listor.neiInP.bias!=0 || listor.neiInG.bias!=0) std::abort();
                result.stats.merge(listor.stats);
#endif
                cand1.clearPrefix(blkSz);cand2.clearPrefix(blkSz);
            }
            results[stream]=result;
        },1);
        unsigned long long cntMaxPlex=0;
        size_t validblk=0,rootCount=0;
        unsigned long long snapshots=0,snapshotCapacityBytes=0;
        for (const Result& result:results) {
            cntMaxPlex+=result.count;validblk+=result.valid;rootCount+=result.roots;
            snapshots+=result.snapshots;snapshotCapacityBytes+=result.snapshotCapacityBytes;
        }
#ifdef P011_DIAGNOSTICS
        MinorityStats stats;
        for(const auto& result:results) stats.merge(result.stats);
        printf("P011 totals: %s\n",stats.overflow ? "INVALID-overflow" : "valid");
        printf("P011 old row entries: %llu\n",(unsigned long long)stats.oldEntries);
        printf("P011 actual row entries: %llu\n",(unsigned long long)stats.newEntries);
        printf("P011 updates: %llu\n",(unsigned long long)stats.updates);
        printf("P011 forks: %llu\n",(unsigned long long)stats.forks);
        printf("P011 P-bias forks: %llu\n",(unsigned long long)stats.pBiasForks);
        printf("P011 G-bias forks: %llu\n",(unsigned long long)stats.gBiasForks);
        printf("P011 view swaps: %llu\n",(unsigned long long)stats.viewSwaps);
        printf("P011 biased view swaps: %llu\n",(unsigned long long)stats.biasedViewSwaps);
        printf("P011 view restores: %llu\n",(unsigned long long)stats.restores);
        if(!stats.overflow && (stats.newEntries>stats.oldEntries || stats.viewSwaps!=stats.restores)) std::abort();
#endif
        const auto done=Clock::now();
        printf("Preprocessing time: %.6f s\n",std::chrono::duration<double>(rootsStart-searchStart).count());
        printf("Root search time: %.6f s\n",std::chrono::duration<double>(done-rootsStart).count());
        printf("Root streams: %zu\n",streams);
        printf("Dense scratch bytes: %llu\n",7ULL*streams*size_t(pn));
        printf("Total valid block %zu\n",validblk);
        printf("Number of %d-plex: %llu\n",k,cntMaxPlex);
        printf("Running time: %.3f s\n",std::chrono::duration<double>(done-searchStart).count());
        printf("Task Cnt: %zu\n",rootCount);
        printf("Fork snapshots: %llu\n",snapshots);
        printf("Snapshot base capacity bytes (cumulative): %llu\n",snapshotCapacityBytes);
    }
int main_maximal_kplex(char* file_path,int k_value,int lower_bound,int binary_flag,double timeLimit){
    std::string input(file_path);
    size_t dot=input.find_last_of('.');
    if(dot==std::string::npos){fprintf(stderr,"Input file needs .txt, .clq or .bin extension\n");return 2;}
    std::string suffix=input.substr(dot+1);
    if(binary_flag){
        if(suffix!="txt" && suffix!="clq"){fprintf(stderr,"Conversion requires .txt or .clq\n");return 2;}
        if(main_toBin(file_path))return 2;
        input.replace(dot+1,std::string::npos,"bin");
        if(k_value==0) return 0;
        suffix="bin";
    }
    if(suffix!="bin" && suffix!="txt"){fprintf(stderr,"Enumeration accepts .bin or .txt; convert .clq with -b\n");return 2;}
    if(k_value<2 || lower_bound<2LL*k_value-1){fprintf(stderr,"Maximal k-plex enumeration requires k>=2 and q>=2k-1\n");return 2;}
    k=k_value;lb=lower_bound;bd=lb-k;time_limit=timeLimit;
    timer IOtime;IOtime.start();
    graph<intT> g = suffix=="bin" ? readBinaryGraph(input.c_str()) : readSNAPText(input.c_str());
    printf("reading time: %.5f ms\n",IOtime.stop()*1000);
    decomposableSearch(g);
    g.del();
    return 0;
}

}
