#pragma once
#include <atomic>
#include <cassert>
#include <cstdint>
#include <limits>

namespace ListPlex {
static_assert(sizeof(int)==4 && sizeof(uint32_t)==4, "P010 word proxy requires 32-bit integer storage");
// Structural compact snapshot capacity in 32-bit-word equivalents. Inputs are
// nonnegative int graph dimensions, so all arithmetic below fits uint64_t.
inline uint64_t snapshotWords(int block, int left) {
    assert(block >= 0 && left >= 0);
    const uint64_t b = uint64_t(block), l = uint64_t(left);
    return 1 + 12*b + 2*l + 4*(b/32+1) + (l/32+1);
}

struct ForkPool {
    const uint64_t capacity;
    // Includes tentative rejected reservations; never interpreted as a clone
    // count. At most P simultaneous callers, hence at most capacity+P.
    std::atomic<uint64_t> reserved{0};
#ifdef P010_DIAGNOSTICS
    std::atomic<uint64_t> attempts{0}, snapshots{0}, denied{0}, words{0};
    std::atomic<uint64_t> siteGrants[3]{};
    std::atomic<uint64_t> exclusionAliasGrants{0};
    std::atomic<uint64_t> meteredWords{0}, fundedWords{0}, renewals{0};
    std::atomic<bool> wordTotalsOverflow{false};
    void recordWords(std::atomic<uint64_t>& total, uint64_t amount) {
        const uint64_t old = total.fetch_add(amount, std::memory_order_relaxed);
        if (old > std::numeric_limits<uint64_t>::max()-amount)
            wordTotalsOverflow.store(true, std::memory_order_relaxed);
    }
#endif
    explicit ForkPool(uint64_t cap) : capacity(cap) {}
    ForkPool(const ForkPool&) = delete;
    ForkPool& operator=(const ForkPool&) = delete;
};

// Declared before clone storage: destruction occurs after that storage and the
// normally completed strict join. Pre-dispatch allocation failures unwind the
// guard; pinned Parlay does not provide exception-safe joining after dispatch.
// No retries, catch-and-continue, or scheduler changes.
class ForkPermit {
    ForkPool* pool;
public:
    ForkPermit(ForkPool* owner, uint64_t cost) : pool(owner) {
        if (!pool) return;
#ifdef P010_DIAGNOSTICS
        pool->attempts.fetch_add(1, std::memory_order_relaxed);
#endif
        if (pool->reserved.fetch_add(1, std::memory_order_relaxed) >= pool->capacity) {
            pool->reserved.fetch_sub(1, std::memory_order_relaxed);
#ifdef P010_DIAGNOSTICS
            pool->denied.fetch_add(1, std::memory_order_relaxed);
#endif
            pool = nullptr;
        }
#ifdef P010_DIAGNOSTICS
        else {
            pool->snapshots.fetch_add(1, std::memory_order_relaxed);
            pool->recordWords(pool->words,cost);
        }
#else
        (void)cost;
#endif
    }
    ~ForkPermit() {
        if (pool) pool->reserved.fetch_sub(1, std::memory_order_relaxed);
    }
    ForkPermit(const ForkPermit&) = delete;
    ForkPermit& operator=(const ForkPermit&) = delete;
    explicit operator bool() const { return pool != nullptr; }
};

struct EarnedWork {
    ForkPool* pool = nullptr;
    uint64_t threshold = 1, credit = 0;
#ifdef P010_DIAGNOSTICS
    bool hasJoinedFork = false; // private logical-state history, not backtracked
#endif
    void earn(uint64_t completed) {
        if (pool) {
#ifdef P010_DIAGNOSTICS
            pool->recordWords(pool->meteredWords,completed);
#endif
            credit += completed < threshold-credit ? completed : threshold-credit;
        }
    }
    bool consumeAttempt() {
        if (!pool || credit < threshold) return false;
        credit = 0; // Failed reservations consume credit too; never backtracked.
#ifdef P010_DIAGNOSTICS
        pool->recordWords(pool->fundedWords,threshold);
#endif
        return true;
    }
};
}
