#ifndef COMMON_H
#define COMMON_H
#include <fstream>
#include <Eigen/Core>
#include <cstdio>
#include <cstdlib>

#include "quicklz.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))


#define GOTO_OFFSET(origin, iter, offset) \
	do { \
	iter = origin + offset; \
	} while(0)

#define GOTO_OFFSET_SUM(origin, iter, increment, sum) \
	do { \
	sum += increment; \
	iter = origin + sum; \
	} while(0)

#define NEXTLINE(iter) \
	while (*iter != '\n') \
		iter++; \
	iter++ \


size_t getfilesize(FILE *file);	
size_t cpp_getfilesize(std::ifstream&);

void printMatrix4f(Eigen::Matrix4f &a);
void printVector4f(const Eigen::Vector4f &a);

char* decompress_qlz(std::ifstream &file, char** buffer);


#endif