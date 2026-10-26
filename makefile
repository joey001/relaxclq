.DEFAULT_GOAL := parlay-include-elision
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

# P013 inherits the exact P005 native scheduler and only elides doomed includes.
P013_PROJECT_ROOT ?= $(abspath $(dir $(firstword $(wildcard ../execution-environments.json ../../execution-environments.json))))
PARLAY_ROOT ?= $(P013_PROJECT_ROOT)/deps/parlaylib
P013_OUT ?= $(P013_PROJECT_ROOT)/build/p013
P013_FLAGS ?= -O3 -DNDEBUG -march=native
P013_SOURCES = maximal_kplex/toBin.cpp maximal_clique/util.cpp maximal_clique/set_operation.cpp maximal_clique/bitpack_maximal_clique.cpp maximum_clique/GraphClique.cpp maximum_kplex/Graph.cpp main.cpp maximal_kplex/listPlex.cpp
P013_HEADERS = $(wildcard maximal_kplex/*.h)
P013_BASE = -std=c++17 -pthread -DPARLAY_ELASTIC_PARALLELISM=0 -msse4.2 -DTARGET -I. -I$(PARLAY_ROOT)/include
P013_ASAN = -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -DP013_DIAGNOSTICS
.PHONY: parlay-include-elision parlay-include-elision-emit parlay-include-elision-check-emit parlay-include-elision-dispatch parlay-include-elision-asan parlay-include-elision-dispatch-asan
parlay-include-elision: $(P013_OUT)/MKP
parlay-include-elision-emit: $(P013_OUT)/MKP-emit
parlay-include-elision-check-emit: $(P013_OUT)/MKP-check-emit
parlay-include-elision-dispatch: $(P013_OUT)/dispatch-check
parlay-include-elision-asan: $(P013_OUT)/candidate-asan-emit
parlay-include-elision-dispatch-asan: $(P013_OUT)/dispatch-asan
$(P013_OUT)/MKP: $(P013_SOURCES) $(P013_HEADERS)
	mkdir -p $(P013_OUT)
	$(CXX) $(P013_BASE) $(P013_FLAGS) $(P013_SOURCES) -o $@
$(P013_OUT)/MKP-emit: $(P013_SOURCES) $(P013_HEADERS)
	mkdir -p $(P013_OUT)
	$(CXX) $(P013_BASE) $(P013_FLAGS) -DLISTPLEX_EMIT_SOLUTIONS $(P013_SOURCES) -o $@
$(P013_OUT)/MKP-check-emit: maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp $(P013_HEADERS)
	mkdir -p $(P013_OUT)
	$(CXX) $(P013_BASE) $(P013_FLAGS) -DP013_DIAGNOSTICS -DLISTPLEX_EMIT_SOLUTIONS maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp -o $@
$(P013_OUT)/dispatch-check: maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p013_dispatch.cpp $(P013_HEADERS)
	mkdir -p $(P013_OUT)
	$(CXX) $(P013_BASE) $(P013_FLAGS) -DP013_DIAGNOSTICS maximal_kplex/toBin.cpp tests/p013_dispatch.cpp -o $@
$(P013_OUT)/candidate-asan-emit: maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp $(P013_HEADERS)
	mkdir -p $(P013_OUT)
	$(CXX) $(P013_BASE) $(P013_ASAN) -DLISTPLEX_EMIT_SOLUTIONS maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p005_ownership_driver.cpp -o $@
$(P013_OUT)/dispatch-asan: maximal_kplex/toBin.cpp maximal_kplex/listPlex.cpp tests/p013_dispatch.cpp $(P013_HEADERS)
	mkdir -p $(P013_OUT)
	$(CXX) $(P013_BASE) $(P013_ASAN) maximal_kplex/toBin.cpp tests/p013_dispatch.cpp -o $@
