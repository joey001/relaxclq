// Validation-only actual predicate/cache and strict-fork lifetime unit.
#include "../maximal_kplex/listPlex.cpp"
#include <numeric>
using namespace ListPlex;
static void require(bool value) { SaturationStats::require(value); }
struct Fixture {
    static constexpr int n=192;
    Graph graph;
    Stack<int> plex{n},exclStack{n};
    VtxSet c1{n},c2{n},excl{n},bk{n};
    std::vector<int> p=std::vector<int>(n,n-1),g=p,ids;
    std::vector<uint8_t> matrix=std::vector<uint8_t>(n*n,1);
    DecomposeMaxChecker checker;
    PlexEmitor emitter;
    KplexListor<DecomposeMaxChecker,PlexEmitor> owner;
    Fixture():graph(static_cast<vertex<int>*>(calloc(n,sizeof(vertex<int>))),n,0),
        checker(graph,0),emitter(ids,ids),owner(graph,&checker,&emitter,&plex,&c1,&c2,&excl,&bk,&exclStack,p.data(),g.data(),n,0,matrix) {}
    void prepare(int size,int pattern) {
        plex.clear();std::fill(p.begin(),p.end(),n-1);std::fill(g.begin(),g.end(),n-1);
        std::fill(matrix.begin(),matrix.end(),1);
        for(int v=0;v<size;++v) { plex.push(v);p[v]=size-k+((pattern==0 || (pattern==2 && v%2))?1:0); }
    }
    void unlink(int v,int u) { matrix[size_t(v)*n+u]=matrix[size_t(u)*n+v]=0; }
};
int main() {
    k=2;lb=3;
    Fixture f;
    for(int size=0;size<=17;++size)for(int pattern=0;pattern<3;++pattern) {
        f.prepare(size,pattern);
        decltype(f.owner)::SaturationPrefix prefix(f.owner);
        require(prefix.query(180,0));require(prefix.cursor==size);
        require(prefix.query(181,1));
    }
    f.prepare(4,2);f.unlink(0,180);f.unlink(2,181);
    {
        decltype(f.owner)::SaturationPrefix prefix(f.owner);
        f.p[179]=0;require(!prefix.query(179,0));
        require(prefix.cursor==0 && f.owner.saturationScratch.empty());
        require(!prefix.query(180,0));require(prefix.cursor==1 && f.owner.saturationScratch==std::vector<int>{0});
        require(!prefix.query(180,0));require(prefix.cursor==1);
        require(!prefix.query(181,0));require(prefix.cursor==3 && f.owner.saturationScratch==std::vector<int>({0,2}));
        require(prefix.query(182,0));require(prefix.cursor==4);
        require(!prefix.query(180,0));require(prefix.cursor==4);
        f.g[182]=0;require(!prefix.query(182,0));require(prefix.cursor==4);
    }
    const size_t retained=f.owner.saturationScratch.capacity();
    f.prepare(1,0);
    {
        decltype(f.owner)::SaturationPrefix prefix(f.owner);
        require(f.owner.saturationScratch.empty() && f.owner.saturationScratch.capacity()==retained);
        require(prefix.query(180,0));
    }
    f.prepare(129,1);
    { decltype(f.owner)::SaturationPrefix prefix(f.owner);require(prefix.query(180,0)); }
    require(f.owner.saturationScratch.size()==129 && f.owner.saturationScratch.capacity()>=129);

    // Exercise the actual P=G append/substitution/restoration code and repeat it.
    f.prepare(2,0);f.c1.add(2);f.c1.add(3);f.excl.add(4);f.unlink(0,4);
    f.g[0]=2;
    int* originalP=f.owner.neiInP;
    for(int rep=0;rep<3;++rep) {
        f.owner.checkAsMaxPlex();
        require(f.owner.neiInP==originalP && f.plex.sz==2 && !f.owner.prefixActive);
    }
    f.c1.clearPrefix(Fixture::n);f.excl.clearPrefix(Fixture::n);

    // A genuine native par_do snapshot: both owners mutate independent caches.
    f.prepare(4,1);
    { decltype(f.owner)::SaturationPrefix prefix(f.owner);require(prefix.query(180,0)); }
    std::atomic<int> credits{1};f.owner.forkCredits=&credits;
    bool childCalled=false,parentCalled=false;
    f.owner.forkBranch([&](auto& child) {
        require(child.saturationScratch.empty() && child.saturationScratch.capacity()==0);
        typename decltype(f.owner)::SaturationPrefix prefix(child);
        require(prefix.query(180,0));childCalled=true;
    },[&] {
        decltype(f.owner)::SaturationPrefix prefix(f.owner);
        require(prefix.query(181,0));parentCalled=true;
    });
    require(parlay::num_workers()==4 && childCalled && parentCalled && credits==0);
    require(f.owner.saturationStats.forks>0 && f.owner.saturationStats.nonemptyParentForks>0);
    SaturationStats overflow;overflow.queries=UINT64_MAX;overflow.add(overflow.queries,1);require(overflow.overflow);
    require(!f.owner.saturationStats.overflow && f.owner.saturationStats.cached+f.owner.saturationStats.scanned<=f.owner.saturationStats.original);
    puts("P012 actual prefix differential, ordered failures, lazy growth, P=G restoration and independent real fork passed");
}
