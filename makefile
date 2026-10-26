.DEFAULT_GOAL := parlay-saturation-prefix
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

# P005 native Parlay root tasks. Override PARLAY_ROOT for another checkout.
P005_PROJECT_ROOT ?= $(abspath $(dir $(firstword $(wildcard ../execution-environments.json ../../execution-environments.json))))
PARLAY_ROOT ?= $(P005_PROJECT_ROOT)/deps/parlaylib
P005_SOURCES = maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp
P005_HEADERS = $(wildcard maximal_kplex/*.h)

# P012 inherits P005 search/runtime and changes only pass-local saturation scans.
P012_OUT ?= $(P005_PROJECT_ROOT)/build/p012
P012_FLAGS ?= -O3 -DNDEBUG -march=native
P012_BASE = -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -I. -I$(PARLAY_ROOT)/include
.PHONY: parlay-saturation-prefix parlay-saturation-prefix-emit parlay-saturation-prefix-check-emit parlay-saturation-prefix-unit
parlay-saturation-prefix: $(P012_OUT)/MKP
parlay-saturation-prefix-emit: $(P012_OUT)/MKP-emit
parlay-saturation-prefix-check-emit: $(P012_OUT)/MKP-check-emit
parlay-saturation-prefix-unit: $(P012_OUT)/prefix-unit
$(P012_OUT)/MKP: $(P005_SOURCES) $(P005_HEADERS)
	mkdir -p $(P012_OUT)
	$(CXX) $(P012_BASE) $(P012_FLAGS) $(P005_SOURCES) -o $@
$(P012_OUT)/MKP-emit: $(P005_SOURCES) $(P005_HEADERS)
	mkdir -p $(P012_OUT)
	$(CXX) $(P012_BASE) $(P012_FLAGS) -DLISTPLEX_EMIT_SOLUTIONS $(P005_SOURCES) -o $@
$(P012_OUT)/MKP-check-emit: maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp $(P005_HEADERS)
	mkdir -p $(P012_OUT)
	$(CXX) $(P012_BASE) $(P012_FLAGS) -DP012_DIAGNOSTICS -DLISTPLEX_EMIT_SOLUTIONS maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp -o $@
$(P012_OUT)/prefix-unit: tests/p012_prefix.cpp maximal_kplex/listPlex.cpp maximal_kplex/toBin.cpp $(P005_HEADERS)
	mkdir -p $(P012_OUT)
	$(CXX) $(P012_BASE) -O1 -g -DP012_DIAGNOSTICS -fsanitize=address,undefined -fno-omit-frame-pointer maximal_kplex/toBin.cpp tests/p012_prefix.cpp -o $@
