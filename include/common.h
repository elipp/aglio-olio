#ifndef COMMON_H
#define COMMON_H

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/time.h>
#include <GL/GL.h>

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


class _timer {

	struct timespec beg;
	struct timespec end;

public:

	void begin() {
		clock_gettime(CLOCK_REALTIME, &beg);
	}
	time_t get_ns() {
		clock_gettime(CLOCK_REALTIME, &end);
		return (end.tv_sec*1000000000 + end.tv_nsec) - (beg.tv_sec*1000000000 + beg.tv_nsec);
	}
	time_t get_us() {
		clock_gettime(CLOCK_REALTIME, &end);
		return ((end.tv_sec*1000000000 + end.tv_nsec) - (beg.tv_sec*1000000000 + beg.tv_nsec))/1000;
	}
	time_t get_ms() {
		clock_gettime(CLOCK_REALTIME, &end);
		return ((end.tv_sec*1000000000 + end.tv_nsec) - (beg.tv_sec*1000000000 + beg.tv_nsec))/1000000;
	}
	double get_s() {
		return ((double)(end.tv_sec*1000000000 + end.tv_nsec) - (double)(beg.tv_sec*1000000000 + beg.tv_nsec))/1000000000.0;

	}
	_timer() { memset(&beg, 0, sizeof(struct timespec)); memset(&end, 0, sizeof(struct timespec)); }

};


size_t getfilesize(FILE *file);	
size_t cpp_getfilesize(std::ifstream&);

char* decompress_qlz(std::ifstream &file, char** buffer);


#endif
