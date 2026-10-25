#include "../maximal_kplex/MRandSet.h"
#include <cstdio>
int main() {
    RandSet<int> set;
    const int sizes[]={0,1,31,32,33,63,64,65,520,3,32,31,33,0,4100,3,64,1,0};
    for(int pass=0;pass<20;++pass) for(int b:sizes) {
        set.resize(b);
        for(int v=0;v<b;++v) {assert(!set.contains(v));set.add(v);}
        // Leave flags for fake-removed suffix elements set: active-only
        // clearing would miss them, whereas the current-universe reset must not.
        while(set.sz>1) set.fakeRemove(set.members[set.sz/2]);
        set.clearPrefix(b);
        assert(set.sz==0 && set.flag->empty());
        for(int v=0;v<set.cap;++v) assert(!set.contains(v));
    }
    std::puts("PREFIX_GROWTH_PASS");
}
