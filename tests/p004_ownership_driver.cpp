// Validation-only direct entry point; does not modify the timed solver.
#include "maximal_kplex/listPlex.h"
#include "maximal_kplex/graphIO.h"
#include <cstdlib>
#include <cstring>
int main(int argc,char** argv) {
  if (argc == 4 && std::strcmp(argv[1], "ownership") == 0) {
    for (int i = 0; i < 20; ++i)
      if (main_toBin(argv[2])) return 2;
    auto graph = ListPlex::readSNAPText(argv[2]);
    ListPlex::writeBinaryGraph(argv[3],graph);
    graph.del(); graph.del();
    auto readback = ListPlex::readBinaryGraph(argv[3]);
    if (readback.n != 8 || readback.m != 56) return 3;
    readback.del(); readback.del();
    return 0;
  }
  if (argc < 4) return 2;
  return ListPlex::main_maximal_kplex(argv[1],std::atoi(argv[2]),std::atoi(argv[3]),argc>4 ? std::atoi(argv[4]) : 0,86400);
}
