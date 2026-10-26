// Abstract admission/lifetime tests, not evidence of graph speedup.
#include "../maximal_kplex/p010_fork_admission.h"
#include <cassert>
#include <limits>
#include <climits>
#include <stdexcept>
using namespace ListPlex;
struct StorageWitness {
    ForkPool& pool;
    ~StorageWitness() { assert(pool.reserved.load() == 1); }
};
int main() {
    assert(snapshotWords(0,0)==6);
    const uint64_t largest=uint64_t(INT_MAX);
    assert(snapshotWords(INT_MAX,INT_MAX)==1+14*largest+5*(largest/32+1));
    assert(snapshotWords(32,32)==1+12*32+2*32+8+2);
    ForkPool pool(1);
    EarnedWork parent; parent.pool=&pool; parent.threshold=37;
    assert(!parent.consumeAttempt());
    parent.earn(0); assert(parent.credit==0);
    parent.earn(1); // nonzero credit before maximum-sized increment
    parent.earn(std::numeric_limits<uint64_t>::max());
    assert(parent.credit==37 && parent.consumeAttempt() && parent.credit==0);
    {
        ForkPermit ancestor(&pool,parent.threshold); assert(ancestor);
        StorageWitness storage{pool}; // must die before ancestor permit
        EarnedWork child; child.pool=&pool; child.threshold=parent.threshold;
        assert(child.credit==0 && !child.consumeAttempt());
        child.earn(37); assert(child.consumeAttempt());
        { ForkPermit denied(&pool,37); assert(!denied); }
        assert(pool.reserved.load()==1 && child.credit==0);
        // A held ancestor denies repeated funded renewals without leaking slots.
        child.earn(37); assert(child.consumeAttempt());
        { ForkPermit denied(&pool,37); assert(!denied); }
        assert(!child.consumeAttempt());
    }
    assert(pool.reserved.load()==0);
    // Renewal after join succeeds, but only newly executed work pays for it.
    assert(!parent.consumeAttempt()); parent.earn(36); assert(!parent.consumeAttempt());
    parent.earn(1); assert(parent.consumeAttempt());
    { ForkPermit renewed(&pool,37); assert(renewed); }
    assert(pool.reserved.load()==0);
    try {
        ForkPermit permit(&pool,37); assert(permit);
        StorageWitness storage{pool};
        throw std::runtime_error("unwind");
    } catch (const std::runtime_error&) {}
    assert(pool.reserved.load()==0);
    EarnedWork sequential; sequential.earn(std::numeric_limits<uint64_t>::max());
    assert(sequential.credit==0 && !sequential.consumeAttempt());
    assert(pool.attempts.load()==5 && pool.snapshots.load()==3 && pool.denied.load()==2);
    assert(pool.words.load()==3*37);
    // Independent logical roots share one process pool, not one pool per root.
    ForkPool global(2);
    {
        ForkPermit rootA(&global,1), rootB(&global,1);
        assert(rootA && rootB && global.reserved.load()==2);
        ForkPermit suspendedDescendant(&global,1);
        assert(!suspendedDescendant && global.reserved.load()==2);
    }
    assert(global.reserved.load()==0);
    // Wrapped diagnostic values must be visibly invalid, never exact totals.
    ForkPool overflow(1);
    overflow.recordWords(overflow.words,std::numeric_limits<uint64_t>::max());
    assert(!overflow.wordTotalsOverflow.load());
    overflow.recordWords(overflow.words,1);
    assert(overflow.wordTotalsOverflow.load());
}
