#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <string>
#include "maximal_kplex/listPlex.h"
#include "maximum_kplex/Graph.h"
#include "maximum_clique/GraphClique.h"
#ifdef TARGET
#include "maximal_clique/bitpack_maximal_clique.hpp"
#endif

void printHelp(){
    puts("Usage: MKP -f graph.bin -m maximal|maximum [-k K] [-l Q] [-t seconds]");
    puts("  -k positive integer (default 1: clique)");
    puts("  -l minimum size; maximal default/minimum 2*k-1; maximum default 0");
    puts("  -t finite time limit in [0,86400] seconds (default 86400)");
    puts("  -b convert .txt/.clq to .bin; omit -m for conversion only");
}
bool parseInteger(const char* value,int minimum,int& result){
    char* end=nullptr;errno=0;long parsed=strtol(value,&end,10);
    if(errno || end==value || *end || parsed<minimum || parsed>INT_MAX/4)return false;
    result=int(parsed);return true;
}
int main(int argc,char** argv){
    char* file=nullptr;char* mode=nullptr;int binary=0,k_value=1,lower=0;
    bool hasLower=false;double limit=86400;int opt;
    while((opt=getopt(argc,argv,"f:bt:k:m:l:h"))!=-1){
        switch(opt){
        case 'f':file=optarg;break;
        case 'b':binary=1;break;
        case 'k':if(!parseInteger(optarg,1,k_value)){fputs("Invalid -k\n",stderr);return 2;}break;
        case 'l':if(!parseInteger(optarg,0,lower)){fputs("Invalid -l\n",stderr);return 2;}hasLower=true;break;
        case 'm':mode=optarg;break;
        case 't':{
            char* end=nullptr;errno=0;limit=strtod(optarg,&end);
            if(errno || end==optarg || *end || !std::isfinite(limit) || limit<0 || limit>86400){fputs("Invalid -t\n",stderr);return 2;}
            break;
        }
        case 'h':printHelp();return 0;
        default:return 2;
        }
    }
    if(optind!=argc || !file || !*file || (!mode && !binary) || (mode && strcmp(mode,"maximal") && strcmp(mode,"maximum"))){printHelp();return 2;}
    if(!strrchr(file,'.')){fputs("Input file extension is missing\n",stderr);return 2;}
    if(!mode)return ListPlex::main_maximal_kplex(file,0,0,1,limit);
    int result=0;
    if(!strcmp(mode,"maximal")){
        int minimum=2*k_value-1;
        if(!hasLower)lower=minimum;
        if(lower<minimum){fputs("Maximal enumeration requires -l >= 2*k-1\n",stderr);return 2;}
        if(k_value>1)result=ListPlex::main_maximal_kplex(file,k_value,lower,binary,limit);
        else {
#ifdef TARGET
            BPMaximalClique::main_maximal_clique(file,lower,binary,limit);
#else
            fputs("This build does not include maximal clique enumeration\n",stderr);return 2;
#endif
        }
    }else if(k_value>1)Graph::main_maximum_kplex(file,k_value,lower,binary,limit);
    else GraphClique::main_maximum_clique(file,lower,binary,limit,mode);
    if(result)return result;
    printf("Time limit: %.3f seconds\nk-value: %d\nType: %s\nLower bound: %d\n",limit,k_value,mode,lower);
    return 0;
}
