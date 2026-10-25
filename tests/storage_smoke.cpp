#include <cassert>
#include <cstring>
#include "maximal_kplex/MRandSet.h"
#include "maximal_kplex/MStack.h"
#include "maximal_kplex/graph.h"
struct Counters { void addG(int) {} };
int main(){
    ListPlex::Stack<int> stack;
    stack.resize(2); stack.push(7); stack.push(9); stack.resize(80000);
    assert(stack.members[0]==7 && stack.members[1]==9);
    RandSet<int> set; set.resize(70000);
    for(int i=0;i<70000;++i)set.add(i);
    set.fakeRemove(12345); set.resize(140000);
    Counters counters;set.fakeRecoverAdd(1,&counters);
    assert(set.sz==70000 && set.contains(12345));
    set.remove(69999);assert(!set.contains(69999));set.add(100000);
    assert(set.contains(100000));int count=0;set.for_each([&](int){++count;});assert(count==70000);
    for(int iteration=0;iteration<1000;++iteration){
        auto vertices=newA(vertex<int>,1);
        vertices[0].Neighbors=newA(int,1);vertices[0].Neighbors[0]=42;
        vertices[0].NeighborsLeft=nullptr;vertices[0].degree=1;
        graph<int> owner(vertices,1,1,nullptr);auto view=owner;
        owner.del();owner.del();assert(view.V[0].Neighbors[0]==42);
        view.del();view.del();
    }
}
