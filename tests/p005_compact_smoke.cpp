#include "../maximal_kplex/MRandSet.h"
#include "../maximal_kplex/MStack.h"
#include <cstdio>
int main() {
 for (int b : {0,1,31,32,33,63,64,65,73,520,4100}) {
  const int historical=b+257;
  RandSet<int> source(historical);
  for(int i=0;i<historical;++i) {source.members[i]=i;source.pos[i]=i;}
  for(int i=0;i<b;++i) source.add(i);
  if(b>=2) {source.fakeRemove(0);source.fakeRemove(1);}
  // Deliberate out-of-universe ghosts must never survive the compact tail.
  source.flag->set(b);source.flag->set(historical-1);
  RandSet<int> child(source,b);
  assert(child.cap==b && child.sz==source.sz && child.flag->cap==b);
  for(int i=0;i<b;++i) {
   assert(child.members[i]==source.members[i] && child.pos[i]==source.pos[i]);
   assert(child.contains(i)==source.contains(i));
  }
  for(int i=b;i<=(b/32)*32+31;++i) assert(!child.flag->test(i));
  if(b>=2) {
   // Active-only flags would have lost these intentional fake removals.
   assert(child.contains(0) && child.contains(1));
   child.fakeRecover(2);assert(child.sz==b);
   for(int i=0;i<b;++i) assert(child.members[child.pos[i]]==i);
  }
  ListPlex::Stack<int> stack(historical);
  for(int i=0;i<b/2;++i) stack.push(i*7);
  ListPlex::Stack<int> compactStack(stack,b);
  assert(compactStack.cap==b && compactStack.sz==stack.sz);
  for(int i=0;i<stack.sz;++i) assert(compactStack.members[i]==stack.members[i]);
  while(compactStack.sz<b) compactStack.push(99);
  while(compactStack.sz) compactStack.pop();
 }
 puts("P005 compact current-universe copies passed");
}
