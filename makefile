.DEFAULT_GOAL := parlay-minority-rows
# avalible targets
target_x86   =   x86
target_arm   =   arm

# bulid target
TARGET_ARCH   =   $(target)

ifeq ($(TARGET_ARCH), $(target_x86))
    FLAG = -g -std=c++14 -mno-sse -msse4.2 -O3 -Ofast -fopenmp -DOPENMP -DNDEBUG -march=native -w -DTARGET
else ifeq ($(TARGET_ARCH), $(target_arm))
    FLAG = -g -std=c++14 -O3 -Ofast -fopenmp -DOPENMP -DNDEBUG -march=native -w
endif
#FLAG = -g -std=c++14 -fopenmp -DOPENMP

ifeq ($(OS),Windows_NT)
    # Windows
    RM = del /Q
else
    # Linux
    RM = rm -f
endif
ifeq ($(OS),Windows_NT)
    # Windows
    OUTPUT = MKP.exe
else
    # Linux
    OUTPUT = MKP
endif
ifdef PROMPT
	FLAG := $(FLAG) -DPROMPT
endif

ifdef OPT
	FLAG := $(FLAG) -Ofast
endif
#-fcilkplus

all: 
    ifeq ($(TARGET_ARCH), $(target_x86))
	    g++ $(FLAG) maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp -o MKP 
    else ifeq ($(TARGET_ARCH), $(target_arm))
	    g++ $(FLAG) maximal_kplex/toBin.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp -o MKP 
    endif
clean:
	 $(RM) $(OUTPUT)

# P011 outputs must be explicitly versioned; no historical build is reused.
P011_PROJECT_ROOT ?= $(abspath ../..)
PARLAY_ROOT ?= $(P011_PROJECT_ROOT)/deps/parlaylib
P011_FLAGS ?= -O3 -DNDEBUG -march=native
P011_SOURCES = maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp
P011_HEADERS = $(wildcard maximal_kplex/*.h)
.PHONY: p011-output-check parlay-minority-rows parlay-minority-rows-emit parlay-minority-rows-diagnostic parlay-minority-rows-unit parlay-minority-rows-check-emit
p011-output-check:
	@test -n "$(P011_OUT)" || (echo 'Set P011_OUT to a fresh build/p011-<revision> directory'; exit 2)
parlay-minority-rows: $(P011_OUT)/MKP
parlay-minority-rows-emit: $(P011_OUT)/MKP-emit
parlay-minority-rows-diagnostic: $(P011_OUT)/MKP-diagnostic
parlay-minority-rows-unit: $(P011_OUT)/counts-unit
$(P011_OUT)/MKP: $(P011_SOURCES) $(P011_HEADERS) | p011-output-check
	mkdir -p $(P011_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET $(P011_FLAGS) -I$(PARLAY_ROOT)/include $(P011_SOURCES) -o $@
$(P011_OUT)/MKP-emit: $(P011_SOURCES) $(P011_HEADERS) | p011-output-check
	mkdir -p $(P011_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -DLISTPLEX_EMIT_SOLUTIONS $(P011_FLAGS) -I$(PARLAY_ROOT)/include $(P011_SOURCES) -o $@
$(P011_OUT)/MKP-diagnostic: $(P011_SOURCES) $(P011_HEADERS) | p011-output-check
	mkdir -p $(P011_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -DP011_DIAGNOSTICS $(P011_FLAGS) -I$(PARLAY_ROOT)/include $(P011_SOURCES) -o $@
$(P011_OUT)/counts-unit: tests/p011_counts.cpp maximal_kplex/listPlex.cpp $(P011_HEADERS) | p011-output-check
	mkdir -p $(P011_OUT)
	$(CXX) -std=c++17 -pthread -O1 -g -DPARLAY_ELASTIC_PARALLELISM=0 -DTARGET -msse4.2 -DP011_DIAGNOSTICS -I$(PARLAY_ROOT)/include -fsanitize=address,undefined -fno-omit-frame-pointer tests/p011_counts.cpp maximal_kplex/toBin.cpp -o $@

parlay-minority-rows-check-emit: $(P011_OUT)/MKP-check-emit
$(P011_OUT)/MKP-check-emit: maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp $(P011_HEADERS) | p011-output-check
	mkdir -p $(P011_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -DP011_DIAGNOSTICS -DLISTPLEX_EMIT_SOLUTIONS $(P011_FLAGS) -I. -I$(PARLAY_ROOT)/include maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp -o $@
