#ifndef _BENCH_GRAPH_IO
#define _BENCH_GRAPH_IO

#include <iostream>
#include <stdint.h>
#include <cstring>
#include "config.h"
#include "toBin.h"
#include "graph.h"
using namespace std;

using ui = unsigned int;
namespace ListPlex{
inline static std::string integer_to_string(long long number) {
	std::vector<ui> sequence;
	if (number == 0) sequence.push_back(0);
	while (number > 0) {
		sequence.push_back(number % 1000);
		number /= 1000;
	}

	char buf[5];
	std::string res;
	for (unsigned int i = sequence.size(); i > 0; i--) {
		if (i == sequence.size()) sprintf(buf, "%u", sequence[i - 1]);
		else sprintf(buf, ",%03u", sequence[i - 1]);
		res += std::string(buf);
	}
	return res;
}
inline graph<intT> readDIMACS2Text(const char* filepath){
    ui n_IO, m_IO;
    ui* pstart_IO;
    ui* edges_IO;
    ui* reverse;
	std::ifstream infile;
	char buf[1024];
	std::vector<std::pair<ui, ui> > epairs;
	std::vector<ui> nodes;
	//FILE *f = Utility::open_file(filepath, "r");
	infile.open(filepath, std::ios::in| std::ios::binary);
	if (!infile.is_open()) {
		fprintf(stderr, "can not find file %s\n", filepath);
		exit(1);
	}
	int max_id = 0;
	int from, to;
	int i=0;
	while (infile.getline(buf, 1024)) {
		char *p = buf;
        while (*p == 'e' && *p != '\0') p++;
		while (*p == ' ' && *p != '\0') p++;
		if (*p == 'p' || *p == '\0'||*p == 'c') continue;
		std::stringstream ss(p);
		ss >> from >> to;
		if (from != to) {
			epairs.push_back(std::make_pair(from, to));
			epairs.push_back(std::make_pair(to, from));
			nodes.push_back(from);
			nodes.push_back(to);
		}
	}
	infile.close();

	sort(nodes.begin(), nodes.end());
	nodes.erase(unique(nodes.begin(), nodes.end()), nodes.end());

	sort(epairs.begin(), epairs.end());
	epairs.erase(unique(epairs.begin(), epairs.end()), epairs.end());

	ui contn = 1;
	std::map<ui, ui> idmp;
	for (ui i = 0; i < nodes.size(); i++) {
		idmp[nodes[i]] = i;
		if (nodes[i] != i) {
			contn = 0;
		}
	}
	//if (contn == 0) printf("Node ids are not preserved! \n");

	n_IO = nodes.size();
	m_IO = epairs.size();
    uintT *in = newA(uintT, n_IO + m_IO + 2);
    in[0] = n_IO;
    in[1] = m_IO;
	printf("n= %s, m  = %s(undirected)\n",
		integer_to_string(n_IO).c_str(),
		integer_to_string(m_IO).c_str());
	pstart_IO = new ui[n_IO + 1];
	edges_IO = new ui[m_IO];
	reverse = new ui[m_IO];
	ui j = 0;
	for (ui i = 0; i < n_IO; i++) {
		pstart_IO[i] = j;
		while (j < m_IO && epairs[j].first == nodes[i]) {
			edges_IO[j] = idmp[epairs[j].second];
			reverse[j] = i;
			++j;
		}
	}
	pstart_IO[n_IO] = j;
    ui *degree = new ui[n_IO];
	for (ui i = 0; i < n_IO; i++)
	degree[i] = pstart_IO[i + 1] - pstart_IO[i];
    vertex<intT> *v = newA(vertex<intT>, n_IO);
    parallel_for(uintT i = 0; i < n_IO; i++)
    {
        uintT d = degree[i];
        uintT o = pstart_IO[i];
        v[i].degree = d;
        v[i].Neighbors = (intT*)(edges_IO+o);
      }
      free(degree);
    return graph<intT>(v, (intT)n_IO, (uintT)m_IO, (intT *)in);
}
inline graph<intT> readSNAPText(const char* filepath) {
    ui n_IO, m_IO;
    ui* pstart_IO;
    ui* edges_IO;
    ui* reverse;
	std::ifstream infile;
	char buf[1024];
	std::vector<std::pair<ui, ui> > epairs;
	std::vector<ui> nodes;
	//FILE *f = Utility::open_file(filepath, "r");
	infile.open(filepath, std::ios::in);
	if (!infile.is_open()) {
		fprintf(stderr, "can not find file %s\n", filepath);
		exit(1);
	}
	int max_id = 0;
	int from, to;
	while (infile.getline(buf, 1024)) {
		char *p = buf;
		while (*p == ' ' && *p != '\0') p++;
		if (*p == '#' || *p == '\0') continue;
		std::stringstream ss(buf);
		ss >> from >> to;
		if (from != to) {
			epairs.push_back(std::make_pair(from, to));
			epairs.push_back(std::make_pair(to, from));
			nodes.push_back(from);
			nodes.push_back(to);
		}
	}
	infile.close();

	sort(nodes.begin(), nodes.end());
	nodes.erase(unique(nodes.begin(), nodes.end()), nodes.end());

	sort(epairs.begin(), epairs.end());
	epairs.erase(unique(epairs.begin(), epairs.end()), epairs.end());

	ui contn = 1;
	std::map<ui, ui> idmp;
	for (ui i = 0; i < nodes.size(); i++) {
		idmp[nodes[i]] = i;
		if (nodes[i] != i) {
			contn = 0;
		}
	}
	//if (contn == 0) printf("Node ids are not preserved! \n");

	n_IO = nodes.size();
	m_IO = epairs.size();
    uintT *in = newA(uintT, n_IO + m_IO + 2);
    in[0] = n_IO;
    in[1] = m_IO;
	printf("n= %s, m  = %s(undirected)\n",
		integer_to_string(n_IO).c_str(),
		integer_to_string(m_IO).c_str());
	pstart_IO = new ui[n_IO + 1];
	edges_IO = new ui[m_IO];
	reverse = new ui[m_IO];
	ui j = 0;
	for (ui i = 0; i < n_IO; i++) {
		pstart_IO[i] = j;
		while (j < m_IO && epairs[j].first == nodes[i]) {
			edges_IO[j] = idmp[epairs[j].second];
			reverse[j] = i;
			++j;
		}
	}
	pstart_IO[n_IO] = j;
    ui *degree = new ui[n_IO];
	for (ui i = 0; i < n_IO; i++)
	degree[i] = pstart_IO[i + 1] - pstart_IO[i];
    vertex<intT> *v = newA(vertex<intT>, n_IO);
    parallel_for(uintT i = 0; i < n_IO; i++)
    {
        uintT d = degree[i];
        uintT o = pstart_IO[i];
        v[i].degree = d;
        v[i].Neighbors = (intT*)(edges_IO+o);
      }
      free(degree);
    return graph<intT>(v, (intT)n_IO, (uintT)m_IO, (intT *)in);
}
inline void writeBinaryGraph(const char *filepath, const graph<intT> &g){
    using ui = unsigned int;
    std::vector<ui> nodes;
    FILE *f = utils::open_file(filepath, "wb");
    ui tt = sizeof(ui);
    fwrite(&tt, sizeof(ui), 1, f); //length of ui
    fwrite(&(g.n), sizeof(ui), 1, f);
    fwrite(&(g.m), sizeof(ui), 1, f);
    ui *degree = new ui[g.n];
    for (ui i = 0; i < g.n; i++)
      degree[i] = g.V[i].degree;
    fwrite(degree, sizeof(ui), g.n, f);
    fwrite(g.allocatedInplace + 2 + g.n, sizeof(ui), g.m, f);
    fclose(f);
  }

  //read graph from binary file
 inline  graph<intT> readBinaryGraph(const char *filepath)
  {
      FILE *f = utils::open_file(filepath, "rb");
      if (f == NULL)
        perror("Error opening file");
      uintT tt;
      utils::fread_wall(&tt, sizeof(uintT), 1, f);
      if (tt != sizeof(uintT))
      {
          printf("sizeof unsigned int is different: file %u, machine %lu\n_IO", tt, sizeof(uintT));
      }
      uintT n_IO, m_IO;
      utils::fread_wall(&n_IO, sizeof(uintT), 1, f); //number of vertices
      utils::fread_wall(&m_IO, sizeof(uintT), 1, f); //number of edges_IO
      //printf("n_IO=%u, m_IO=%u (undirected)\n", n_IO, m_IO);

      uintT *in = newA(uintT, n_IO + m_IO + 2);
      in[0] = n_IO;
      in[1] = m_IO;
      uintT *offset = in + 2;
      uintT *edges_IO = in + 2 + n_IO;

      //degree
      uintT *degree = newA(uintT, n_IO);
      utils::fread_wall(degree, sizeof(uintT), n_IO, f);
      //if (reverse != nullptr) delete[] reverse;
      //reverse = new ui[m_IO];
      uintT sum = 0;
      for (uintT i = 0; i < n_IO; i++)
      {
          offset[i] = (i == 0) ? 0 : offset[i - 1] + degree[i - 1];
          utils::fread_wall(edges_IO + offset[i], sizeof(uintT), degree[i], f);
      }

      vertex<intT> *v = newA(vertex<intT>, n_IO);

      parallel_for(uintT i = 0; i < n_IO; i++)
      {
          uintT d = degree[i];
          uintT o = offset[i];
          v[i].degree = d;
          v[i].Neighbors = (intT *)edges_IO + o;
      }
      fclose(f);
      free(degree);
      return graph<intT>(v, (intT)n_IO, (uintT)m_IO, (intT *)in);
  }
 inline  void checkGraph(graph<int> &g){
        int ecnt = 0;
        for (int i = 0; i < g.n; i++){
            printf("Nei [%d]:", i);
            for (int j = 0; j < g.V[i].degree; j++){
                int nei = g.V[i].Neighbors[j];
                ecnt++;
                if ( j>0 ) assert(nei > g.V[i].Neighbors[j-1]);
                printf("%d ", nei);
                auto r= find(g.V[nei].Neighbors, g.V[nei].Neighbors + g.V[nei].degree, i);
                assert(r != g.V[nei].Neighbors + g.V[nei].degree);
            } 
            printf("\n_IO");
        }
        assert(ecnt == g.m);
    }
}

#endif // _BENCH_GRAPH_IO
