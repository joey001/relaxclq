#ifndef TOBIN_H 
#define TOBIN_H
#include <cassert>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <map>
#include <cstring>
#include "utils.h"
using ui = unsigned int;
static int fileSuffixPos(char* filepath);
static std::string integer_to_string(long long number);
void readDIMACS2Text(const char* filepath);
void readRawSNAPText(const char* filepath);
int readRawDIM10Text(const char* filepath);
int writeBinaryGraph_ToBin(const char* filepath);
void promot();
int main_toBin(char* filePath);
#endif