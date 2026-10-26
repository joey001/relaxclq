// Exercise the actual listByCase/kSearch methods against externally supplied
// independent-oracle sets. No duplicate implementation of the dispatch rule.
#include "../maximal_kplex/listPlex.cpp"
#include <fstream>
#include <set>
#include <sstream>
using namespace ListPlex;
struct Collected {
    std::mutex lock;
    std::vector<std::vector<int>> rows;
};
struct CollectEmitor {
    unsigned long long count=0;
    Collected* output;
    void emitPlex(const int* list,int size) {
        std::vector<int> row(list,list+size);std::sort(row.begin(),row.end());
        std::lock_guard<std::mutex> guard(output->lock);
        output->rows.push_back(std::move(row));++count;
    }
};
static void require(bool value) { ElisionStats::require(value); }
static std::vector<int> active(VtxSet& set) {
    std::vector<int> result(set.members,set.members+set.sz);
    std::sort(result.begin(),result.end());
    for(int i=0;i<set.sz;++i) require(set.pos[set.members[i]]==i);
    return result;
}
int main(int argc,char** argv) {
    if(argc!=7)return 2;
    k=std::atoi(argv[2]);lb=std::atoi(argv[3]);bd=lb-k;
    const bool equality=std::string(argv[5])=="equality";
    const bool grant=std::string(argv[6])=="grant";
    auto graph=readBinaryGraph(argv[1]);const int n=graph.n;
    std::vector<uint8_t> matrix(size_t(n)*n);
    std::vector<int> leftDummy(n),p(n),g(n);
    for(int v=0;v<n;++v) {
        graph.V[v].NeighborsLeft=leftDummy.data()+v;graph.V[v].degreeLeft=0;
        for(int j=0;j<graph.V[v].degree;++j)matrix[size_t(v)*n+graph.V[v].Neighbors[j]]=1;
    }
    std::set<std::vector<int>> expected;
    std::ifstream input(argv[4]);std::string line;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::vector<int> vertices;int v;
        while(row>>v)vertices.push_back(v);
        if(!vertices.empty())expected.insert(vertices);
    }
    require(input.eof() && !expected.empty());
    Stack<int> plex(n),stack(n);VtxSet c1(n),c2(n),x(n),bk(n);plex.push(0);
    for(int v=1;v<n;++v)if(matrix[v])c1.add(v);else c2.add(v);
    for(int v=0;v<n;++v) {
        p[v]=matrix[size_t(v)*n];g[v]=p[v];
        for(int j=0;j<c1.sz;++j)g[v]+=matrix[size_t(v)*n+c1.members[j]];
        graph.V[v].degreeHop=g[v];
    }
    const auto originalP=p,originalG=g,originalC1=active(c1),originalC2=active(c2);
    if(equality) require(c2.sz==0 && k==2 && lb==5);
    else {
        require(c2.sz>0 && p[0]+k>plex.sz);
        bool low=false;for(int j=0;j<c1.sz;++j)low|=g[c1.members[j]]<bd;
        require(low);
        // The supplied oracle must contain an actual extension using C2.
        bool selectableSolution=false;
        for(const auto& row:expected)for(int v:originalC2)
            selectableSolution|=std::binary_search(row.begin(),row.end(),v);
        require(selectableSolution);
    }
    DecomposeMaxChecker checker(graph,0);Collected collected;CollectEmitor emitter{0,&collected};
    KplexListor<DecomposeMaxChecker,CollectEmitor> owner(graph,&checker,&emitter,&plex,&c1,&c2,&x,&bk,&stack,p.data(),g.data(),1+c1.sz,c2.sz,matrix);
    ElisionStats totals;
    for(int rep=0;rep<3;++rep) {
        collected.rows.clear();emitter.count=0;owner.elisionStats={};
        std::atomic<int> credits(grant ? n-1 : 0);
        owner.forkCredits=parlay::num_workers()>1 ? &credits : nullptr;
        owner.kSearch(k-1);
        std::set<std::vector<int>> actual(collected.rows.begin(),collected.rows.end());
        require(actual==expected && actual.size()==collected.rows.size() && emitter.count==expected.size());
        require(p==originalP && g==originalG && active(c1)==originalC1 && active(c2)==originalC2);
        require(plex.sz==1 && plex.members[0]==0 && x.sz==0 && bk.sz==0 && stack.sz==0);
        require(!owner.elisionStats.overflow && owner.elisionStats.degreeChecks>0 && owner.elisionStats.checks>0);
        if(equality)require(owner.elisionStats.equalities>0);else require(owner.elisionStats.elisions>0);
        require(owner.elisionStats.elisions==owner.elisionStats.restores);
        if(grant && parlay::num_workers()>1)require(owner.elisionStats.granted>0);
        else require(owner.elisionStats.granted==0 && owner.elisionStats.ungranted>0);
        totals.merge(owner.elisionStats);
    }
    ElisionStats overflow;overflow.visits=UINT64_MAX;overflow.add(overflow.visits);require(overflow.overflow);
    printf("Parlay workers: %zu\n",parlay::num_workers());
    printf("P013 direct repetitions: 3\n");
    for(auto row:collected.rows) {printf("SOLUTION");for(int v:row)printf(" %d",v);puts("");}
    printf("Number of %d-plex: %llu\n",k,emitter.count);totals.print();
}
