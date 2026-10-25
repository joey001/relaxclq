#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "MBitSet.h"
#include "config.h"


template <typename T=int> 
class RandSet{    
public:
    using TIndex = int;
    int sz;
    int cap;
    T *members;
    TIndex *pos; 
    MBitSet32 *flag;
    
public:

    //default constructor        
    RandSet():sz(0),cap(0),members(nullptr),pos(nullptr),flag(new MBitSet32(0)){}

    RandSet(int _maxvalue):cap(_maxvalue),sz(0){       
        members = new T[_maxvalue];
        pos = new TIndex[_maxvalue];
        flag = new MBitSet32(_maxvalue);
    }

    
    RandSet(const RandSet &_set){// copy constructor
        cap = _set.cap;
        sz = _set.sz;
        
        members = new T[cap];
        memcpy(members, _set.members, sizeof(T) * sz);

        pos = new TIndex[cap];
        memcpy(pos, _set.pos, sizeof(TIndex) * cap);

        flag = new MBitSet32(*(_set.flag));        
        
    }
    // Compact snapshot of the current root universe, not retained capacity.
    // Keep removed suffix/positions and fake-remove flag semantics verbatim.
    RandSet(const RandSet& source,int universe)
        : sz(source.sz),cap(universe),members(new T[universe]),
          pos(new TIndex[universe]),flag(new MBitSet32(universe)) {
        assert(universe>=0 && universe<=source.cap && sz<=universe);
        if (universe) {
            memcpy(members,source.members,sizeof(T)*universe);
            memcpy(pos,source.pos,sizeof(TIndex)*universe);
        }
        memcpy(flag->buf,source.flag->buf,sizeof(uint32_t)*(flag->n+1));
        const unsigned tail=unsigned(universe)&31U;
        flag->buf[flag->n] &= tail ? ((uint32_t(1)<<tail)-1) : 0U;
    }
    RandSet& operator=(const RandSet &_set) = delete;

    ~RandSet(){
        delete[] members;
        delete[] pos;
        delete flag;        
    }

    void resize(const int _cap){
        if (_cap <= cap) return;
        T *grownMembers = new T[_cap];
        TIndex *grownPos = new TIndex[_cap];
        // Preserve the removed suffix: backtracking restores it without reordering.
        if (cap) {
            memcpy(grownMembers, members, sizeof(T) * cap);
            memcpy(grownPos, pos, sizeof(TIndex) * cap);
        }
        flag->reserve(_cap);
        delete[] members; delete[] pos;
        members = grownMembers; pos = grownPos; cap = _cap;
    }

    bool contains(const T& v){
        assert(v<cap);        
        return flag->test(v);
    }

    void add(const T& v){
        pos[v] = sz; 
        members[sz++]=v;
        flag->set(v); 
    }

    void fakeRemove(const T& v){
        //swap
        TIndex idx = pos[v];            
        pos[members[sz-1]] = idx;
        pos[v] = sz-1;
        std::swap(members[idx],members[--sz]);
    }

    void remove(const T& v){
        fakeRemove(v);
        flag->set(v);
    }

    template<typename Type>
    void recoverAdd(const int len,Type* listor){
        T* cursor=members+sz;
        for(int i=0;i<len;++i){
            const int u=*cursor;
            flag->set(u);
            listor->addG(u);
            cursor++;
        }
        sz+=len;
    }

    template<typename Type>
    void fakeRecoverAdd(const int len,Type* listor){
        T* cursor=members+sz;
        for(int i=0;i<len;++i){
            const int ele=*cursor;
            listor->addG(ele);
            cursor++;
        }
        sz+=len;
    }

    void fakeRecover(const int len){
        sz+=len;
    }
     
    int pop_back(){
        int v=members[--sz];
        flag->set(v);
        return v;
    }

    RandSet& operator+(const T& v){
        add(v);
        return *this;
    }
    
    void clear(){
        flag->clear();
        sz = 0;                
    }

    // At a root boundary only [0,universe) may have changed membership.
    // A prior larger root cleared its own universe; growth zeroes new words.
    void clearPrefix(int universe){
        assert(universe >= 0 && universe <= cap);
        flag->clearPrefix(universe);
        sz = 0;
    }

    void clearByEle(){
        for(int i=0;i<sz;++i){
            flag->set(members[i]);
        }
        sz = 0;                
    }

    template<typename Fnc>
    void for_each(const Fnc &f){        
        for (TIndex i = 0; i < sz; i++){
            f(members[i]);
        }
    }
};