.DEFAULT_GOAL := parlay-root-streams
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

# P004 native Parlay root tasks. Override PARLAY_ROOT for another checkout.
P004_PROJECT_ROOT ?= $(abspath $(dir $(firstword $(wildcard ../execution-environments.json ../../execution-environments.json))))
PARLAY_ROOT ?= $(P004_PROJECT_ROOT)/deps/parlaylib
P004_OUT ?= $(P004_PROJECT_ROOT)/build/p004
P004_FLAGS ?= -O3 -DNDEBUG -march=native
P004_SOURCES = maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp
P004_HEADERS = $(wildcard maximal_kplex/*.h)
.PHONY: parlay-root-streams parlay-root-streams-emit
parlay-root-streams: $(P004_OUT)/MKP
parlay-root-streams-emit: $(P004_OUT)/MKP-emit
$(P004_OUT)/MKP: $(P004_SOURCES) $(P004_HEADERS)
	mkdir -p $(P004_OUT)
	$(CXX) -std=c++17 -pthread -msse4.2 -DTARGET $(P004_FLAGS) -I$(PARLAY_ROOT)/include $(P004_SOURCES) -o $@
$(P004_OUT)/MKP-emit: $(P004_SOURCES) $(P004_HEADERS)
	mkdir -p $(P004_OUT)
	$(CXX) -std=c++17 -pthread -msse4.2 -DTARGET -DLISTPLEX_EMIT_SOLUTIONS $(P004_FLAGS) -I$(PARLAY_ROOT)/include $(P004_SOURCES) -o $@
