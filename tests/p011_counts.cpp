// Validation-only inclusion exposes the actual private maximality checker.
#include "../maximal_kplex/listPlex.cpp"
#include <numeric>
using namespace ListPlex;
using Edges=std::vector<std::pair<int,int>>;
static void require(bool b){if(!b)std::abort();}
static void differential(int n,const Edges& edges) {
    std::vector<std::vector<int>> adj(n);
    for(auto [u,v]:edges){adj[u].push_back(v);adj[v].push_back(u);}
    for(auto& row:adj)std::sort(row.begin(),row.end());
    UpdateRows rows(n);
    for(int u=0;u<n;++u){
        int* closed=rows.initializeRow(u,int(adj[u].size()),adj[u].data());int cursor=0;
        for(int v=0;v<n;++v)if(!std::binary_search(adj[u].begin(),adj[u].end(),v)){
            if(closed)closed[cursor++]=v;
        }
        require(rows.dense(u)==(int(adj[u].size())>n-int(adj[u].size())));
        require(rows.length(u)==std::min(int(adj[u].size()),n-int(adj[u].size())));
    }
    for(int h=0;h<=n;++h){
        std::vector<int> p(n,0),g(n,0),wantP(n,0),wantG(n,0);
        for(int u=0;u<h;++u)for(int v:adj[u])++g[v];wantG=g;
        ShiftedCounts P{p.data(),0},G{g.data(),0};
        auto update=[&](ShiftedCounts& view,std::vector<int>& truth,int u,int sign){
            if(sign==1)rows.update<1>(view,u);else rows.update<-1>(view,u);
            for(int v:adj[u])truth[v]+=sign;
            for(int v=0;v<n;++v)require(view[v]==truth[v] && view.raw[v]>=-n && view.raw[v]<=n);
            require(view.bias>=-n && view.bias<=n);
        };
        for(int u=0;u<n;++u)update(P,wantP,u,1);
        auto rawChild=p;ShiftedCounts child{rawChild.data(),P.bias};auto childTruth=wantP;
        if(n){update(child,childTruth,0,-1);for(int v=0;v<n;++v)require(P[v]==wantP[v]);}
        ShiftedCounts saved=P;P=G;
        for(int v=0;v<n;++v)require(P[v]==wantG[v]);P=saved;
        require(P.raw==saved.raw && P.bias==saved.bias);
        for(int u=n;u-->0;)update(P,wantP,u,-1);
        require(P.bias==0);
        for(int v=0;v<n;++v)require(child[v]==childTruth[v]);
        for(int u=0;u<h;++u)update(G,wantG,u,-1);
        for(int u=0;u<n;++u)update(G,wantG,u,1);
        for(int u=n;u-->0;)update(G,wantG,u,-1);
        for(int u=0;u<h;++u)update(G,wantG,u,1);
        require(G.bias==0);
    }
}
int main(){
    MinorityStats saturated, merged;
    saturated.oldEntries=std::numeric_limits<uint64_t>::max();
    saturated.add(saturated.oldEntries,1);require(saturated.overflow);
    merged.merge(saturated);require(merged.overflow);
    for(int n=0;n<=9;++n){
        Edges empty,clique,path,star,cycle;
        for(int u=0;u<n;++u)for(int v=u+1;v<n;++v)clique.emplace_back(u,v);
        for(int u=1;u<n;++u){path.emplace_back(u-1,u);star.emplace_back(0,u);}
        cycle=path;if(n>2)cycle.emplace_back(0,n-1);
        for(const auto* e:{&empty,&clique,&path,&star,&cycle})differential(n,*e);
    }
    // Clique: initial H=V, remove all G. Raw may equal +b, not just b-1.
    const int n=5;UpdateRows clique(n);std::vector<int> raw(n,n-1);
    for(int u=0;u<n;++u){int* out=clique.initializeRow(u,n-1,nullptr);require(out);out[0]=u;}
    ShiftedCounts G{raw.data(),0};for(int u=0;u<n;++u)clique.update<-1>(G,u);
    require(G.bias==-n);for(int v=0;v<n;++v)require(raw[v]==n && G[v]==0);
    // Actual left-maximality checker: path a-c-b, x adjacent to b,c only.
    vertex<int>* vertices=newA(vertex<int>,3);
    const std::vector<std::vector<int>> adj={{2},{2},{0,1}},left={{},{0},{0}};
    for(int u=0;u<3;++u){
        vertices[u].degree=int(adj[u].size());vertices[u].degreeHop=vertices[u].degree;
        vertices[u].Neighbors=newA(int,2);std::copy(adj[u].begin(),adj[u].end(),vertices[u].Neighbors);
        vertices[u].degreeLeft=int(left[u].size());vertices[u].NeighborsLeft=newA(int,1);
        std::copy(left[u].begin(),left[u].end(),vertices[u].NeighborsLeft);
    }
    Graph graph(vertices,3,0,nullptr);DecomposeMaxChecker checker(graph,1);
    k=2;lb=3;int list[]={0,1,2};int residual[]={0,0,1};
    require(checker.isMaximal(list,3,{residual,1}));
    int logical[]={1,1,2};require(checker.isMaximal(list,3,{logical,0}));
    puts("P011 differential count views, clone independence, whole-view substitution and actual left checker passed");
}
