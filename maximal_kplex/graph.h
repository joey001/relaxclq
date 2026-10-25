#ifndef _GRAPH_INCLUDED
#define _GRAPH_INCLUDED

#include <iostream>
#include <algorithm>
#include <memory>
#include "utils.h"
#include "config.h"

template <class intT>
struct vertex
{
  intT *Neighbors;
  intT *NeighborsLeft;
  intT degree,degreeHop,degreeLeft;
  void del() {
    if(Neighbors!=nullptr)
    free(Neighbors);
    if(NeighborsLeft)free(NeighborsLeft);
  }
};

template <class intT>
struct graph
{
  vertex<intT> *V;
  bool proper;
  const intT n;
  intT m;   
  intT *allocatedInplace;
  // graph copies used by the checker/search are shared views of these buffers.
  std::shared_ptr<vertex<intT>> storage;
  static std::shared_ptr<vertex<intT>> own(vertex<intT>* vertices, intT count, intT* packed) {
    return std::shared_ptr<vertex<intT>>(vertices, [count, packed](vertex<intT>* p) {
      if (packed) free(packed);
      else for (intT i=0; i<count; ++i) p[i].del();
      free(p);
    });
  }

  graph():V(nullptr),proper(false),n(0),m(0),allocatedInplace(nullptr){};
  graph(vertex<intT> *VV, intT nn, uintT mm)
      : V(VV), proper(false), n(nn), m(mm), allocatedInplace(NULL), storage(own(VV,nn,nullptr)) {}
  graph(vertex<intT> *VV, intT nn, uintT mm, intT *ai)
      : V(VV), proper(false), n(nn), m(mm), allocatedInplace(ai), storage(own(VV,nn,ai)) {}
  graph(vertex<intT> *VV, intT nn, uintT mm, intT *ai, double _proper)
      : V(VV), proper(_proper), n(nn), m(mm), allocatedInplace(ai), storage(own(VV,nn,ai)){}
  graph copy()
  {
    vertex<intT> *VN = newA(vertex<intT>, n);
    intT *_allocatedInplace = newA(intT, n + m + 2);
    _allocatedInplace[0] = n;
    _allocatedInplace[1] = m;
    intT *Edges = _allocatedInplace + n + 2;
    intT k = 0;
    for (intT i = 0; i < n; i++) //for each vertex
    {
      _allocatedInplace[i + 2] = allocatedInplace[i + 2]; //copy 
      VN[i] = V[i];
      VN[i].Neighbors = Edges + k;
      for (intT j = 0; j < V[i].degree; j++)
        Edges[k++] = V[i].Neighbors[j];
    }
    return graph(VN, n, m, _allocatedInplace);
  }
  void del()
  {
    storage.reset();
    V = nullptr;
    allocatedInplace = nullptr;
  }

  bool isAdj(intT v1, intT v2) const
  {
      if(V[v1].degree>V[v2].degree)
          std::swap(v1, v2);
      return std::binary_search(V[v1].Neighbors, V[v1].Neighbors + V[v1].degree, v2);
  }
  bool isAdj11(intT v1, intT v2) const
  {
      if(V[v1].degreeHop>V[v2].degreeHop)
          std::swap(v1, v2);
      return std::binary_search(V[v1].Neighbors, V[v1].Neighbors + V[v1].degreeHop, v2);
  }
};

#endif // _GRAPH_INCLUDED
