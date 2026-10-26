#ifndef LISTPLEX_H
#define LISTPLEX_H
#if !defined(PARLAY_ELASTIC_PARALLELISM) || PARLAY_ELASTIC_PARALLELISM
#error "P005 requires fixed native PARLAY_ELASTIC_PARALLELISM=0"
#endif
#if defined(_OPENMP) || defined(OPENMP) || defined(PARLAY_OPENMP) || defined(PARLAY_CILKPLUS) || defined(PARLAY_OPENCILK) || defined(PARLAY_TBB) || defined(PARLAY_SEQUENTIAL)
#error "P005 requires the native Parlay scheduler"
#endif
#include <parlay/parallel.h>
extern int k,lb,bd;
extern double time_limit;
#include <cassert>
#include <functional>
#include <atomic>
#include <memory>
#include "config.h"
#include "timer.h"
#include "MRandSet.h"
#include "MStack.h"
#include "degeneracy.h"
#include "graphIO.h"
#include "sequence.h"
#include <mutex>
#include <cstdlib>
#include <iomanip>
extern std::mutex coutMutex;
extern timer watch;
namespace ListPlex{
    int main_maximal_kplex(char* file_path,int k_value,int lower_bound,int binary_flag,double time_limit);
    void checkTimeLimit();
    using VtxSet = RandSet<int>;
    using Graph = graph<int>;
    enum : uint8_t{
        //odd for linked
        UNLINK2LESS=0,
        LINK2LESS=1,
        UNLINK2EQUAL=2,
        LINK2EQUAL=3,
        UNLINK2MORE=4,
        LINK2MORE=5
    };
    struct PlexEmitor;
    struct DecomposeMaxChecker;

#ifdef P012_DIAGNOSTICS
    struct SaturationStats {
        uint64_t queries=0,cheap=0,cached=0,scanned=0,original=0,pushes=0;
        uint64_t rejectNew=0,rejectCached=0,extensions=0,passes=0,pgPasses=0;
        uint64_t forks=0,nonemptyParentForks=0;
        bool overflow=false;
        void add(uint64_t& dst,uint64_t value) {
            if (UINT64_MAX-dst<value) overflow=true; else dst+=value;
        }
        void merge(const SaturationStats& s) {
            overflow|=s.overflow;
#define P012_MERGE(x) add(x,s.x)
            P012_MERGE(queries);P012_MERGE(cheap);P012_MERGE(cached);
            P012_MERGE(scanned);P012_MERGE(original);P012_MERGE(pushes);
            P012_MERGE(rejectNew);P012_MERGE(rejectCached);P012_MERGE(extensions);
            P012_MERGE(passes);P012_MERGE(pgPasses);P012_MERGE(forks);
            P012_MERGE(nonemptyParentForks);
#undef P012_MERGE
        }
        static void require(bool value) { if (!value) std::abort(); }
    };
#endif

    template <typename MaximalityChecker, typename Emitor>
    struct KplexListor
    {
        const Graph subg;
        MaximalityChecker *plexMaxChecker;
        Emitor *emitor;
        
        int * neiInP;
        int * const neiInG;
        Stack<int> * const plex;
        VtxSet * const cand1;
        VtxSet * const cand2;
        VtxSet *excl;
        VtxSet * const exclBK;
        Stack<int> * const exclStack;
        const int hopSz, hop2Sz;
        const vector<uint8_t>& commonMtx;
        vector<std::pair<int,int> > vp;
        std::atomic<int>* forkCredits=nullptr;
        // Only capacity survives a pass. A constructed fork child starts empty.
        vector<int> saturationScratch;
#ifdef P012_DIAGNOSTICS
        SaturationStats saturationStats;
        bool prefixActive=false;
#endif

        struct SaturationPrefix {
            KplexListor& owner;
            int cursor=0;
            explicit SaturationPrefix(KplexListor& listor,bool pg=false):owner(listor) {
                owner.saturationScratch.clear();
#ifdef P012_DIAGNOSTICS
                SaturationStats::require(!owner.prefixActive);
                owner.prefixActive=true;
                owner.saturationStats.add(owner.saturationStats.passes,1);
                if (pg) owner.saturationStats.add(owner.saturationStats.pgPasses,1);
#endif
            }
            SaturationPrefix(const SaturationPrefix&)=delete;
            SaturationPrefix& operator=(const SaturationPrefix&)=delete;
            ~SaturationPrefix() {
#ifdef P012_DIAGNOSTICS
                owner.prefixActive=false;
#endif
            }
            bool query(const int u,const int extra) {
#ifdef P012_DIAGNOSTICS
                auto& stats=owner.saturationStats;
                uint64_t a=0,e=0,r=0;
                const int oldCursor=cursor;
                const size_t oldSize=owner.saturationScratch.size();
                const size_t oldCapacity=owner.saturationScratch.capacity();
                const bool expected=owner.originalCanFormPlex(u,extra,r);
                auto finish=[&](bool answer,bool cheap=false) {
                    SaturationStats::require(answer==expected && a+e<=r);
                    if (cheap) SaturationStats::require(a==0 && e==0 && cursor==oldCursor &&
                        owner.saturationScratch.size()==oldSize && owner.saturationScratch.capacity()==oldCapacity);
                    size_t index=0;
                    SaturationStats::require(cursor>=0 && cursor<=owner.plex->sz);
                    for (int i=0;i<cursor;++i) {
                        const int v=owner.plex->members[i];
                        if (owner.neiInP[v]+k==owner.plex->sz) {
                            SaturationStats::require(index<owner.saturationScratch.size() && owner.saturationScratch[index]==v);
                            ++index;
                        }
                    }
                    SaturationStats::require(index==owner.saturationScratch.size());
                    stats.add(stats.queries,1);stats.add(stats.cached,a);stats.add(stats.scanned,e);stats.add(stats.original,r);
                    if (cheap) stats.add(stats.cheap,1);
                    if (oldCursor>0 && cursor>oldCursor) stats.add(stats.extensions,1);
                    return answer;
                };
#define P012_RETURN(answer,cheap) return finish(answer,cheap)
#else
#define P012_RETURN(answer,cheap) return answer
#endif
                // G changes during candidate filtering; these tests stay live.
                if (owner.neiInP[u]+k<owner.plex->sz+1 || owner.neiInG[u]+k<max(lb,owner.plex->sz)+extra) {
                    P012_RETURN(false,true);
                }
                for (const int v:owner.saturationScratch) {
                    assert(v!=u);
#ifdef P012_DIAGNOSTICS
                    ++a;
#endif
                    if (!owner.isAdjMtx(v,u)) {
#ifdef P012_DIAGNOSTICS
                        stats.add(stats.rejectCached,1);
#endif
                        P012_RETURN(false,false);
                    }
                }
                while (cursor<owner.plex->sz) {
                    const int v=owner.plex->members[cursor++];
                    assert(v!=u);
#ifdef P012_DIAGNOSTICS
                    ++e;
#endif
                    if (owner.neiInP[v]+k==owner.plex->sz) {
                        // Include a rejecting vertex in the known prefix.
                        owner.saturationScratch.push_back(v);
#ifdef P012_DIAGNOSTICS
                        stats.add(stats.pushes,1);
#endif
                        if (!owner.isAdjMtx(v,u)) {
#ifdef P012_DIAGNOSTICS
                            stats.add(stats.rejectNew,1);
#endif
                            P012_RETURN(false,false);
                        }
                    }
                }
                P012_RETURN(true,false);
#undef P012_RETURN
            }
        };

        KplexListor(Graph &_subg,MaximalityChecker *_plexMaxChecker, Emitor *_emitor,
                    Stack<int> *_plex, VtxSet *_cand1, VtxSet *_cand2, VtxSet *_excl, VtxSet *_exclBK, Stack<int> *_exclStack,
                    int *_neiInP, int *_neiInG, int _hopSz, int _hop2Sz, const vector<uint8_t>& _commonMtx)
        :subg(_subg),plexMaxChecker(_plexMaxChecker),emitor(_emitor)
        ,plex(_plex),cand1(_cand1),cand2(_cand2),excl(_excl),exclBK(_exclBK),exclStack(_exclStack)
        ,neiInP(_neiInP),neiInG(_neiInG),hopSz(_hopSz),hop2Sz(_hop2Sz),commonMtx(_commonMtx)
        {
        }

        // A root owns at most b-1 snapshots in total. The immutable graph and
        // pair matrix are shared; no worker/TLS state is borrowed across par_do.
        template<class First, class Rest>
        void forkBranch(First first, Rest rest) {
            bool granted=false;
            if (forkCredits && parlay::num_workers()>1) {
                int left=forkCredits->load(std::memory_order_relaxed);
                while (left>0 && !(granted=forkCredits->compare_exchange_strong(
                        left,left-1,std::memory_order_relaxed))) {}
            }
            if (!granted) { first(*this); rest(); return; }
            Stack<int> childPlex(*plex,subg.n),childStack(*exclStack,subg.n);
            VtxSet childCand1(*cand1,subg.n),childCand2(*cand2,subg.n),childExcl(*excl,subg.n),childBK(*exclBK,subg.n);
            std::vector<int> childP(neiInP,neiInP+subg.n),childG(neiInG,neiInG+subg.n);
            MaximalityChecker checker(subg,plexMaxChecker->uni.cap);
            Emitor output(*emitor); output.count=0;
            Graph childGraph=subg;
            KplexListor child(childGraph,&checker,&output,&childPlex,&childCand1,&childCand2,
                (excl==exclBK ? &childBK : &childExcl),&childBK,&childStack,childP.data(),childG.data(),hopSz,hop2Sz,commonMtx);
            child.forkCredits=forkCredits;
#ifdef P012_DIAGNOSTICS
            SaturationStats::require(!prefixActive && !child.prefixActive && child.saturationScratch.empty() && child.saturationScratch.capacity()==0);
            saturationStats.add(saturationStats.forks,1);
            if (!saturationScratch.empty()) saturationStats.add(saturationStats.nonemptyParentForks,1);
#endif
            parlay::par_do([&]{first(child);},[&]{rest();});
            emitor->count+=output.count;
#ifdef P012_DIAGNOSTICS
            SaturationStats::require(!prefixActive && !child.prefixActive &&
                (saturationScratch.capacity()==0 || child.saturationScratch.capacity()==0 || saturationScratch.data()!=child.saturationScratch.data()));
            saturationStats.merge(child.saturationStats);
#endif
        }

        size_t getIdx(const int v1,const int v2){
            return size_t(v1)*subg.n+v2;
        }

        bool isAdjMtx(const int v1,const int v2)
        {
            return commonMtx[getIdx(v1,v2)]&1;
        }

        void addG(const int u){
            for (int i = 0; i < subg.V[u].degree; i++)
            {
                const int nei = subg.V[u].Neighbors[i];
                neiInG[nei]++;
            }
        }
        void subG(const int u){
            for (int i = 0; i < subg.V[u].degree; i++)
            {
                const int nei = subg.V[u].Neighbors[i];
                neiInG[nei]--;
            }
        }
        void addP(const int u){
            for (int i = 0; i < subg.V[u].degree; i++)
            {
                const int nei = subg.V[u].Neighbors[i];
                neiInP[nei]++;
            }
        }        
        void subP(const int u){
            for (int i = 0; i < subg.V[u].degree; i++)
            {
                const int nei = subg.V[u].Neighbors[i];
                neiInP[nei]--;
            }
        }

        KplexListor* cand1ToPlex(const int v)
        {
            plex->push(v);
            cand1->remove(v);
            addP(v);
            return this;
        }

        KplexListor* plexToCand1()
        {
            assert(plex->sz > 0);
            const int u = plex->top();
            cand1->add(u);
            plex->pop();
            subP(u);
            return this;
        }

        int cand2BackToPlex()
        {
            const int v=cand2->pop_back();
            plex->push(v);
            addP(v);
            return v;
        }

        int cand2BackToPlexInc()
        {
            const int v=cand2->pop_back();
            plex->push(v);
            addP(v);
            addG(v);
            return v;
        }

        KplexListor* plexToCand2()
        {
            assert(plex->sz > 0);
            const int u = plex->top();
            cand2->add(u);
            plex->pop();
            subP(u);
            return this;
        }

        KplexListor* plexToCand2Inc()
        {
            assert(plex->sz > 0);
            const int u = plex->top();
            cand2->add(u);
            plex->pop();
            subP(u);
            subG(u);
            return this;
        }

        KplexListor* cand1ToExcl(const int v)
        {
            cand1->remove(v);
            excl->add(v);
            subG(v);
            return this;
        }

        KplexListor* exclToCand1(const int v)
        {
            excl->remove(v);
            cand1->add(v);
            addG(v);
            return this;
        }

        int cand2BackToExcl()
        {
            const int u=cand2->pop_back();
            excl->add(u);
            subG(u);
            return u;
        }

        int cand2BackToExclInc()
        {
            const int u=cand2->pop_back();
            excl->add(u);
            return u;
        }

        KplexListor* exclToCand2(const int v)
        {
            excl->remove(v);
            cand2->add(v);
            addG(v);
            return this;
        }

        KplexListor* exclToCand2Inc(const int v)
        {
            excl->remove(v);
            cand2->add(v);
            return this;
        }

        KplexListor* updateExcl(int& recExcl,const int v2add){
            SaturationPrefix prefix(*this);
            recExcl=excl->sz;
            for (int i = 0; i < excl->sz;)
            {
                const int ele = excl->members[i];
                if((subg.proper&&UNLINK2MORE>commonMtx[getIdx(ele,v2add)])||!prefix.query(ele,1)){
                    excl->remove(ele);
                    exclStack->push(ele);
                }
                else ++i;
            }
            recExcl-=excl->sz;
            return this;
        }
        
        KplexListor* updateExclK(int& recExcl,const int v2add){
            recExcl=excl->sz;
            for (int i = 0; i < excl->sz;)
            {
                int ele = excl->members[i];
                if(subg.proper&&UNLINK2MORE>commonMtx[getIdx(ele,v2add)]){
                    excl->remove(ele);
                    exclStack->push(ele);
                }
                else ++i;
            }
            recExcl-=excl->sz;
            return this;
        }

        KplexListor* updateCand1(int& recCand1,const int v2add){
            SaturationPrefix prefix(*this);
            recCand1=cand1->sz;
            for (int i = 0; i < cand1->sz;)
            {
                const int ele = cand1->members[i];
                if((subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(ele,v2add)])||!prefix.query(ele,0)){
                    cand1->remove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand1-=cand1->sz;
            return this;
        }

        KplexListor* updateCand1Fake(int& recCand1,const  int v2add){
            SaturationPrefix prefix(*this);
            recCand1=cand1->sz;
            for (int i = 0; i < cand1->sz;)
            {
                int ele = cand1->members[i];
                if((subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)])||!prefix.query(ele,0)){
                    cand1->fakeRemove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand1-=cand1->sz;
            return this;
        }

        KplexListor* updateCand1KFake(int& recCand1,const  int v2add){
            recCand1=cand1->sz;
            for (int i = 0; i < cand1->sz;)
            {
                int ele = cand1->members[i];
                if(subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)]){
                    cand1->fakeRemove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand1-=cand1->sz;
            return this;
        }

        KplexListor* updateCand2Fake(int& recCand2,const int v2add){
            recCand2=cand2->sz;
            for (int i = 0; i < cand2->sz;)
            {
                int ele = cand2->members[i];
                if(subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)]){
                    cand2->fakeRemove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand2-=cand2->sz;
            return this;
        }

        KplexListor* updateCand2FakeInc(int& recCand2,const int v2add){
            recCand2=cand2->sz;
            for (int i = 0; i < cand2->sz;)
            {
                int ele = cand2->members[i];
                if(subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)]){
                    cand2->fakeRemove(ele);
                }
                else ++i;
            }
            recCand2-=cand2->sz;
            return this;
        }

#ifdef P012_DIAGNOSTICS
        // Independent original predicate, compiled only for validation.
        bool originalCanFormPlex(const int u,const int extra,uint64_t& visits)
        {
            if (neiInP[u] + k < plex->sz + 1 || neiInG[u] + k < max(lb,plex->sz)+extra)
                return false;
            for (int i = 0; i < plex->sz; i++)
            {
                const int v = plex->members[i];
                ++visits;
                assert(v != u);
                if (neiInP[v] + k == plex->sz && !isAdjMtx(v, u))
                { //v is saturated by v,u are not neighbors
                    return false;
                }
            }
            return true;
        }
#endif
        void kSearch(const int res){
            checkTimeLimit();
            int recExcl=0,recExclTmp;
            std::vector<int> recCand1(size_t(res)+1), recCand2(size_t(res)+1);
            if(cand2->sz==0){
                listByCase();
                return;
            }
            if(k>2 && bound12()) return;
            // Partition exactly as serial kSearch: exclude the current back
            // vertex, versus the ordered include-prefix branches below.
            forkBranch([&](KplexListor& child) {
                const int v=child.cand2BackToExclInc();
                child.kSearch(res);
                child.exclToCand2Inc(v);
            },[&] {
            int v2delete;
            //start from the 1st br.
            int br=1;
            for (; br<res; br++)
            {
                const int v2add=cand2BackToPlexInc();
                updateCand1KFake(recCand1[br],v2add);
                updateCand2FakeInc(recCand2[br],v2add);
                updateExclK(recExclTmp,v2add);
                recExcl+=recExclTmp;
                if(cand2->sz){
                    v2delete = cand2BackToExclInc();
                    kSearch(res-br);
                    exclToCand2Inc(v2delete);
                }
                else{
                    kSearch(res-br);
                    break;
                }
            }
            //The last branch.
            if(br==res){
                recCand2[br]=0;
                const int v2add=cand2BackToPlexInc();
                VtxSet* tmp=excl;
                excl=exclBK;
                updateCand1Fake(recCand1[br],v2add);
                listByCase();
                excl=tmp;
            }
            for (int i = br; i >=1 ; i--)
            {
                cand1->fakeRecoverAdd(recCand1[i],this);
                cand2->fakeRecover(recCand2[i]);
                plexToCand2Inc();
            }
            for(int i=0;i<recExcl;++i){
                excl->add(exclStack->top());
                exclStack->pop();
            }
            });
        }

        //Check if plex+cand is a k-plex
        void checkAsMaxPlex()
        {
            memcpy(plex->members + plex->sz, cand1->members, cand1->sz * sizeof(int));
            plex->sz += cand1->sz;
            int *tmp = neiInP;
            neiInP = neiInG;
            //assume plex+cand as a plex

            bool flag = true;
            {
            SaturationPrefix prefix(*this,true);
            for (int i = 0; i < excl->sz;++i)
            {
                if (prefix.query(excl->members[i],1))
                {
                    flag = false;
                    break;
                }
            }
            }
            if (flag&&plexMaxChecker->isMaximal(plex->members, plex->sz, neiInP))
            {
                emitor->emitPlex(plex->members, plex->sz);
            }
            plex->sz -= cand1->sz;
            neiInP = tmp;
        }

        void branchInCandBase(const int pivot)
        {
            // Existing include/exclude partition; each half has a complete,
            // independent exclusion state and global-maximality checker.
            forkBranch([&](KplexListor& child) {
                int recCand1,recExcl;
                child.cand1ToPlex(pivot)->updateCand1Fake(recCand1,pivot)
                    ->updateExcl(recExcl,pivot)->listByCase();
                child.cand1->fakeRecoverAdd(recCand1,&child);
                child.plexToCand1();
                while(recExcl--) {child.excl->add(child.exclStack->top());child.exclStack->pop();}
            },[&] {
                cand1ToExcl(pivot)->listByCase();
                exclToCand1(pivot);
            });
        }

        void branchInPlexBase(const int pivot){
            int tot = k + neiInP[pivot] - plex->sz;
            assert(tot > 0);
            std::vector<int> addList(tot);
            std::vector<int> recCand1(tot);
            int recExcl=0,recExclTmp;
            int sz = 0;
            //Filter cand1 and find non-adjacent nodes
            for (VtxSet::TIndex i = 0; i < cand1->sz; i++)
            {
                const int u = cand1->members[i];
                if (!isAdjMtx(pivot, u))
                {
                    addList[sz++] = u;
                }
                if (sz == tot)
                    break;
            }
            assert(sz == tot);
            //There are at most tol+1 branches.
            forkBranch([&](KplexListor& child) {
                const int excluded=addList[0];
                child.cand1ToExcl(excluded)->listByCase();
                child.exclToCand1(excluded);
            },[&] {
            int v2delete;
            //Proceed br1 in [1,tol-1); there are at least 2 branches
            //add in addList[0,...,br), remove addList[br].
            int v2add;
            int movals = 0; // recording the number of vertices pushing into kplex

            //start from the 1st br.
            int br = 1;
            for (; br < tot; br++)
            {
                v2add = addList[br - 1];
                v2delete = addList[br];
                //if v2add has not been reduced by feasibility condition, i.e., v2add in cand1
                //or the graph size is above lb. stop
                if (cand1->contains(v2add) && plex->sz + cand1->sz >= lb) 
                {
                    //move v2 add to plex and update cand1
                    cand1ToPlex(v2add)->updateCand1(recCand1[movals],v2add)->updateExcl(recExclTmp,v2add);
                    recExcl+=recExclTmp;
                    movals +=1; //mark the number of pushed vertices
                    
                    if (cand1->contains(v2delete))
                    {
                        //if v2delete is still in cand1, remove it ;
                        cand1ToExcl(v2delete);
                        listByCase();
                        exclToCand1(v2delete);
                        //recover the remove vertices
                    }
                    else
                    {
                        listByCase();
                    }
                }
                else
                {
                    break;
                }
            }
            //The last (tolth) branch.
            if (br == tot && cand1->contains(addList[br - 1]) && plex->sz + cand1->sz >= lb)
            {
                v2add = addList[tot - 1];
                cand1ToPlex(v2add)->updateCand1(recCand1[movals],v2add)->updateExcl(recExclTmp,v2add); //all vertices in addList has moved to plex        
                recExcl+=recExclTmp;
                movals += 1;
                listByCase();
            }
            //recover cand1 and excl
            //pop the last br vertices
            movals--;
            while(movals>=0){
                cand1->recoverAdd(recCand1[movals],this);
                plexToCand1();
                movals--;
            }
            while(recExcl){
                excl->add(exclStack->top());
                exclStack->pop();
                recExcl--;
            }
            });
        }

        bool bound1() {
            vp.clear();
            for(int i = 0;i < plex->sz;i ++) vp.push_back(std::make_pair(k-(plex->sz-neiInP[plex->members[i]]), plex->members[i]));
            // for(ui i = 0;i < P_end;i ++) vp.push_back(std::make_pair(-(neiInG[PC[i]]-neiInP[PC[i]]), PC[i]));
            sort(vp.begin(), vp.end());
            int UB = plex->sz, cursor = 0;
            for(int i = 0;i < (int)vp.size(); i++) {
                int u = vp[i].second;
                if(vp[i].first == 0) continue;
                int count = 0;
                for(int j = cursor;j < cand1->sz;j ++) if(!isAdjMtx(u,cand1->members[j])) {
                    if(j != cursor + count){
                        std::swap(cand1->members[j], cand1->members[cursor+count]);
                        cand1->pos[cand1->members[j]] = j;
                        cand1->pos[cand1->members[cursor+count]] = cursor+count;
                    }
                    ++ count;
                }
                int t_ub = count;
                if(vp[i].first < t_ub) t_ub = vp[i].first;
                if(UB + t_ub < lb) {
                    UB += t_ub;
                    cursor += count;
                }
                else return false;
            }
            return UB+(cand1->sz-cursor)<lb;
        }

        bool bound12() {
            vp.clear();
            for(int i = 0;i < plex->sz;i ++) vp.push_back(std::make_pair(k-(plex->sz-neiInP[plex->members[i]]), plex->members[i]));
            sort(vp.begin(), vp.end());
            int UB = plex->sz, cursor1 = 0, cursor2 = 0;
            for(int i = 0;i < (int)vp.size(); i++) {
                int u = vp[i].second;
                if(vp[i].first == 0) continue;
                int count1 = 0, count2 = 0;
                for(int j = cursor1;j < cand1->sz;j ++) if(!isAdjMtx(u,cand1->members[j])) {
                    if(j != cursor1 + count1){
                        std::swap(cand1->members[j], cand1->members[cursor1+count1]);
                        cand1->pos[cand1->members[j]] = j;
                        cand1->pos[cand1->members[cursor1+count1]] = cursor1+count1;
                    }
                    ++ count1;
                }
                for(int j = cursor2;j < cand2->sz;j ++) if(!isAdjMtx(u,cand2->members[j])) {
                    if(j != cursor2 + count2){
                        std::swap(cand2->members[j], cand2->members[cursor2+count2]);
                        cand2->pos[cand2->members[j]] = j;
                        cand2->pos[cand2->members[cursor2+count2]] = cursor2+count2;
                    }
                    ++ count2;
                }
                int t_ub = count1 + count2;
                if(vp[i].first < t_ub) t_ub = vp[i].first;
                if(UB + t_ub < lb) {
                    UB += t_ub;
                    cursor1 += count1;
                    cursor2 += count2;
                }
                else return false;
            }
            return UB+(cand1->sz-cursor1)+(cand2->sz-cursor2)<lb;
        }

        void listByCase()
        {
            if (plex->sz + cand1->sz < lb){
                return;
            }
            if (cand1->sz == 0)
            {
                if (excl->sz == 0 && plexMaxChecker->isMaximal(plex->members, plex->sz, neiInP))
                {
                    emitor->emitPlex(plex->members, plex->sz);
                }
                return;
            }
            if(k>2 && bound1()) return;
            int minnei=INT_MAX;int pivot;
            auto maxswap = [&](int u) {
                if (neiInG[u] < minnei)
                {
                    minnei = neiInG[u];
                    pivot = u;
                }
            };
            plex->for_each(maxswap);
            const int minneiPlex=minnei;
            if(minneiPlex + k < max(lb,plex->sz))
                return;
            cand1->for_each(maxswap);
            //TODO: for better branching prediction:change to switch
            
            if (minnei >= plex->sz + cand1->sz - k)
            {
                checkAsMaxPlex();   //P+C is a k-plex
                return;
            }
            // P005 exposes a bounded strict frontier inside each independent root.
            if (minneiPlex==minnei) branchInPlexBase(pivot);
            else branchInCandBase(pivot);
        }

        void printContextInfo(){
            printf("\n\n[DEPTH]: %d\n", plex->sz);
            printf("PLEX:");
            plex->for_each([](int v){printf("%d ",v);});
            printf("\nCAND1:");
            cand1->for_each([](int v){printf("%d ",v);});
            printf("\nCAND2:");
            cand2->for_each([](int v){printf("%d ",v);});
            printf("\nEXCLUSIVE:");
            excl->for_each([](int v){printf("%d ",v);});
            printf("\n");
            
            for (int i = 0; i < subg.n; i++)
                printf("[%d]%d:%d ", i, neiInP[i], neiInG[i]);
            printf("\n");
        }
    };
}
#endif
