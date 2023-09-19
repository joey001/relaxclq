#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "maximal_kplex/listPlex.h"
#include "maximum_kplex/Graph.h"
#include "maximum_clique/GraphClique.h"
#include "maximal_clique/bitpack_maximal_clique.hpp"
using namespace std;
void printHelp(){
    printf("-f Path to the input file graph\n");
    printf("-b transform Convert a text file to a binary file, no argument\n");
    printf("-t limit time, second\n");
    printf("-k k-value,default 1\n");
    printf("-m if need to find Maximum, argument is Maximum, and if need to enumerate maximal, argument is Maximal\n");
    printf("-l lower bound, if type is Maximal, default 2k-1, and can not lower than default\n");
    printf("if type is Maximum, default 0, and can not lower than default\n");
    printf("if input invalid value, they will be change to default value without information\n");
}
int main(int argc, char **argv){
    char *file_path = NULL;
    int binary_flag = 0;
    double time_limit = 86400;
    int k_value = 1;
    char *type = NULL;
    int lower_bound = 0;
    bool help=false;

    int opt;
    while ((opt = getopt(argc, argv, "f:bt:k:m:l:h")) != -1) {
        switch (opt) {
            case 'f':
                file_path = optarg;
                break;
            case 'b':
                binary_flag = 1;
                break;
            case 't':
                time_limit = strtod(optarg,NULL);
                if(time_limit<0) time_limit=0;
                break;
            case 'k':
                k_value = atoi(optarg);
                if(k_value<=0) k_value=1; 
                break;
            case 'm':
                type = optarg;
                break;
            case 'l':
                lower_bound = atoi(optarg);
                break;
            case 'h':
                printHelp();
                help=true;
                break;
            default:
                fprintf(stderr, "input .MKP/ -h to get help\n");
                return 1;
        }
    }
    if (!help&&(file_path == NULL || type == NULL || !(strcmp(type,"Maximal")==0||strcmp(type,"Maximum")==0))) {
        fprintf(stderr, "Error: -f, and -m are lack or invalid\n");
        return 1;
    }
    if(time_limit>86400){
        fprintf(stderr, "time_limit is too large\n");
        return 1;
    }
    if (file_path != NULL) {
        if(strcmp(type,"Maximal")==0){
            if(lower_bound<2*k_value-1)
            lower_bound=2*k_value-1;
            if(k_value>1)
            ListPlex::main_maximal_kplex(file_path,k_value,lower_bound,binary_flag,time_limit);
            else{
            BPMaximalClique::main_maximal_clique(file_path,lower_bound,binary_flag,time_limit);
            }
        }else if(strcmp(type,"Maximum")==0){
            if(lower_bound<0)
            lower_bound=0;
            if(k_value>1)
            {
            Graph::main_maximum_kplex(file_path,k_value,lower_bound,binary_flag,time_limit);
            }else
            GraphClique::main_maximum_clique(file_path,lower_bound,binary_flag,time_limit,type);

        }
    }

    if (time_limit != 86400 && time_limit>=0) {
        printf("Time limit: %.3f seconds\n", time_limit);
    }
    if(k_value!=1)
    printf("k-value: %d\n", k_value);

    if (type != NULL) {
        printf("Type: %s\n", type);
    }

    if (lower_bound >= 0) {
        printf("Lower bound: %d\n", lower_bound);
    }
    return 0;
}
