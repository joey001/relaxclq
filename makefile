.DEFAULT_GOAL := parlay-dense-frontier
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
P005_OUT ?= $(P005_PROJECT_ROOT)/build/p005
P005_FLAGS ?= -O3 -DNDEBUG -march=native
P005_SOURCES = maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp
P005_HEADERS = $(wildcard maximal_kplex/*.h)
.PHONY: parlay-dense-frontier parlay-dense-frontier-emit
parlay-dense-frontier: $(P005_OUT)/MKP
parlay-dense-frontier-emit: $(P005_OUT)/MKP-emit
$(P005_OUT)/MKP: $(P005_SOURCES) $(P005_HEADERS)
	mkdir -p $(P005_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET $(P005_FLAGS) -I$(PARLAY_ROOT)/include $(P005_SOURCES) -o $@
$(P005_OUT)/MKP-emit: $(P005_SOURCES) $(P005_HEADERS)
	mkdir -p $(P005_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -DLISTPLEX_EMIT_SOLUTIONS $(P005_FLAGS) -I$(PARLAY_ROOT)/include $(P005_SOURCES) -o $@
