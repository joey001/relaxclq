.DEFAULT_GOAL := parlay-earned-work
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

# P010 outputs must be explicitly versioned; no historical build is reused.
P010_PROJECT_ROOT ?= $(abspath ../..)
PARLAY_ROOT ?= $(P010_PROJECT_ROOT)/deps/parlaylib
P010_FLAGS ?= -O3 -DNDEBUG -march=native
P010_SOURCES = maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp
P010_HEADERS = $(wildcard maximal_kplex/*.h)
.PHONY: p010-output-check parlay-earned-work parlay-earned-work-emit parlay-earned-work-diagnostic parlay-earned-work-unit
p010-output-check:
	@test -n "$(P010_OUT)" || (echo 'Set P010_OUT to a fresh build/p010-<revision> directory'; exit 2)
parlay-earned-work: $(P010_OUT)/MKP
parlay-earned-work-emit: $(P010_OUT)/MKP-emit
parlay-earned-work-diagnostic: $(P010_OUT)/MKP-diagnostic
parlay-earned-work-unit: $(P010_OUT)/admission-unit
$(P010_OUT)/MKP: $(P010_SOURCES) $(P010_HEADERS) | p010-output-check
	mkdir -p $(P010_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET $(P010_FLAGS) -I$(PARLAY_ROOT)/include $(P010_SOURCES) -o $@
$(P010_OUT)/MKP-emit: $(P010_SOURCES) $(P010_HEADERS) | p010-output-check
	mkdir -p $(P010_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -DLISTPLEX_EMIT_SOLUTIONS $(P010_FLAGS) -I$(PARLAY_ROOT)/include $(P010_SOURCES) -o $@
$(P010_OUT)/MKP-diagnostic: $(P010_SOURCES) $(P010_HEADERS) | p010-output-check
	mkdir -p $(P010_OUT)
	$(CXX) -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -DP010_DIAGNOSTICS $(P010_FLAGS) -I$(PARLAY_ROOT)/include $(P010_SOURCES) -o $@
$(P010_OUT)/admission-unit: tests/p010_admission.cpp maximal_kplex/p010_fork_admission.h | p010-output-check
	mkdir -p $(P010_OUT)
	$(CXX) -std=c++17 -pthread -O1 -g -DP010_DIAGNOSTICS -fsanitize=address,undefined -fno-omit-frame-pointer $< -o $@
