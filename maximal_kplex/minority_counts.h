#pragma once
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace ListPlex {
// The entire view is task-private. Copy/swap pointer AND bias together.
struct ShiftedCounts {
    int* raw;
    int bias = 0;
    int operator[](int v) const { return raw[v] + bias; }
};

// One immutable table per root; sparse rows borrow local graph adjacency.
// Only dense closed complements own storage. No table is copied by a fork.
class UpdateRows {
    struct Row {
        const int* vertices = nullptr;
        std::unique_ptr<int[]> storage;
        int length = 0;
        bool complement = false;
    };
    const int universe;
    std::vector<Row> rows;
    static size_t checkedSize(int b) {
        if(b<0) throw std::invalid_argument("Negative P011 universe");
        return size_t(b);
    }
public:
    explicit UpdateRows(int b) : universe(b), rows(checkedSize(b)) {}
    UpdateRows(const UpdateRows&) = delete;
    UpdateRows& operator=(const UpdateRows&) = delete;
    int* initializeRow(int u, int degree, const int* neighbors) {
        assert(u>=0 && u<universe && degree>=0 && degree<universe);
        Row& row=rows[size_t(u)];
        row.complement=degree>universe-degree; // ties retain original adjacency
        row.length=row.complement ? universe-degree : degree;
        if (!row.complement) { row.vertices=neighbors; return nullptr; }
        if (size_t(row.length)>std::numeric_limits<size_t>::max()/sizeof(int))
            throw std::length_error("P011 complement allocation overflow");
        row.storage=std::make_unique<int[]>(size_t(row.length));
        row.vertices=row.storage.get();
        return row.storage.get();
    }
    int length(int u) const { return rows[size_t(u)].length; }
    bool dense(int u) const { return rows[size_t(u)].complement; }
    template<int Sign> void update(ShiftedCounts& counts,int u) const {
        static_assert(Sign==1 || Sign==-1,"P011 update sign");
        const Row& row=rows[size_t(u)];
        if (row.complement) {
            counts.bias+=Sign;
            for (int i=0;i<row.length;++i) counts.raw[row.vertices[i]]-=Sign;
        } else {
            for (int i=0;i<row.length;++i) counts.raw[row.vertices[i]]+=Sign;
        }
    }
};

#ifdef P011_DIAGNOSTICS
// Private counters, merged only after strict joins. No release instrumentation.
struct MinorityStats {
    uint64_t oldEntries=0,newEntries=0,updates=0,forks=0,pBiasForks=0,gBiasForks=0;
    uint64_t viewSwaps=0,biasedViewSwaps=0,restores=0;
    bool overflow=false;
    void add(uint64_t& field,uint64_t value) {
        if (field>std::numeric_limits<uint64_t>::max()-value) overflow=true;
        field+=value;
    }
    void merge(const MinorityStats& other) {
        overflow=overflow||other.overflow;
        add(oldEntries,other.oldEntries);add(newEntries,other.newEntries);
        add(updates,other.updates);add(forks,other.forks);
        add(pBiasForks,other.pBiasForks);add(gBiasForks,other.gBiasForks);
        add(viewSwaps,other.viewSwaps);add(biasedViewSwaps,other.biasedViewSwaps);
        add(restores,other.restores);
    }
};
#endif
}
