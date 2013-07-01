#ifndef UINTEGER_H
#define UINTEGER_H

#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>

struct top_uinteger {
    bool is_negative;
    uint64_t value; 
};

struct top_uinteger top_init_uinteger(uint64_t value, bool is_negative);

struct top_uinteger top_sub_uinteger(const struct top_uinteger *a, const struct top_uinteger *b);

bool top_humanize_uinteger(char *buf, size_t bufsize, const struct top_uinteger i);

bool top_sprint_uinteger(char *buf, size_t bufsize, struct top_uinteger i);

struct top_uinteger top_uinteger_calc_result(uint64_t now, uint64_t prev,
					     uint64_t beg);

/* 
 * These return true in the case of a buffer overflow. 
 * If the value has changed since the previous sample, 
 * they will display a + or - to the right of the sample.
 */
bool top_uinteger_format_result(char *buf, size_t bufsize, uint64_t now,
				uint64_t prev, uint64_t beg);

bool top_uinteger_format_mem_result(char *buf, size_t bufsize, uint64_t now,
				    uint64_t prev, uint64_t beg);

#endif /*UINTEGER_H*/
