/*
 * mui_utils.c
 *
 * Copyright (C) 2023 Michel Pollet <buserror@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <time.h>
#include "mui.h"

#ifdef _WIN32
#include <windows.h>
mui_time_t
mui_get_time()
{
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER count = {0};
    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    printf("%lld\n", count.QuadPart*MUI_TIME_SECOND/freq.QuadPart);
    return (mui_time_t) count.QuadPart*MUI_TIME_SECOND/freq.QuadPart;
}
#else
mui_time_t
mui_get_time()
{
	struct timespec tim;
	clock_gettime(CLOCK_MONOTONIC_RAW, &tim);
	uint64_t time = ((uint64_t)tim.tv_sec) * (1000000 / MUI_TIME_RES) +
						tim.tv_nsec / (1000 * MUI_TIME_RES);
	return time;
}
#endif


uint32_t
mui_hash(
	const char * inString )
{
	if (!inString)
		return 0;
	/* Described http://papa.bretmulvey.com/post/124027987928/hash-functions */
	const uint32_t p = 16777619;
	uint32_t hash = 0x811c9dc5;
	while (*inString)
		hash = (hash ^ (*inString++)) * p;
	hash += hash << 13;
	hash ^= hash >> 7;
	hash += hash << 3;
	hash ^= hash >> 17;
	hash += hash << 5;
	return hash;
}