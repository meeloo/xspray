/*
 * Copyright (c) 2008 Apple Computer, Inc.  All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <libutil.h>
#include "uinteger.h"
#include "preferences.h"

struct top_uinteger top_init_uinteger(uint64_t value, bool is_negative) {
    struct top_uinteger r;

    r.is_negative = is_negative;
    r.value = value;

    return r;
}

struct top_uinteger top_sub_uinteger(const struct top_uinteger *a, const struct top_uinteger *b) {
    struct top_uinteger r;

    if(!a->is_negative && !b->is_negative) {
	if(a->value > b->value) {
	    /* The value will fit without underflow. */
	    r.is_negative = false;
	    r.value = a->value - b->value;
	} else {
	    /* B is larger or we have a r.value of 0. */
	    r.is_negative = true;
	    r.value = b->value - a->value;
	}
    } else if (a->is_negative && !b->is_negative) {
	/* A is negative and B is positive. */
	r.is_negative = true;
	/* 
	 * The A value is negative, so actually add the amount we would subtract.
	 * Thus if a is -5 and b is 2: -5 - 2 = -7; 
	 */
	r.value = a->value + b->value;
    } else if(!a->is_negative && b->is_negative) {
	/* A is positive and b is negative. */
	r.is_negative = false;
	/*
	 * If say A is 2 and b is -3 we want value to be: 2 - -3 = 5;
	 */
	r.value = a->value + b->value;
    } else {
	/* They are both negative. */
	r.is_negative = true;
	r.value = a->value + b->value;
    }

    if(0 == r.value)
	r.is_negative = 0;

    return r;
}

/* Return true if an error occurred. */
bool top_humanize_uinteger(char *buf, size_t bufsize, 
			   struct top_uinteger i) {
    
    if(i.is_negative) {
	if(-1 == humanize_number(buf + 1, bufsize - 1, 
				 (int64_t)i.value, "", 
				 HN_AUTOSCALE, HN_NOSPACE | HN_B)) {
	    return true;
	}
	buf[0] = '-';
    } else {
	if(-1 == humanize_number(buf, bufsize,
				 (int64_t)i.value, "",
				 HN_AUTOSCALE, HN_NOSPACE | HN_B)) {
	    return true;
	}
    }

    return false;
}

bool top_sprint_uinteger(char *buf, size_t bufsize,
			 struct top_uinteger i) {
    
    if(i.is_negative) {
	if(-1 == snprintf(buf, bufsize, "-%" PRIu64, i.value))
	    return true;
    } else {
	if(-1 == snprintf(buf, bufsize, "%" PRIu64, i.value))
	    return true;
    }

    return false;
}

struct top_uinteger top_uinteger_calc_result(uint64_t now, uint64_t prev, 
					     uint64_t beg) {
    struct top_uinteger result, prevu, begu;
    
    result = top_init_uinteger(now, false);

    switch(top_prefs_get_mode()) {
    case STATMODE_ACCUM:
	begu = top_init_uinteger(beg, false);
	result = top_sub_uinteger(&result, &begu);
	break;

    case STATMODE_DELTA:
    	prevu = top_init_uinteger(prev, false);
	result = top_sub_uinteger(&result, &prevu);
	break;
    }

    return result;    
}

/* Return true if an error occurred. */
bool top_uinteger_format_result(char *buf, size_t bufsize, uint64_t now,
				uint64_t prev, uint64_t beg) {
    struct top_uinteger i;
    int suffix = '\0';

    i = top_uinteger_calc_result(now, prev, beg);

    if(STATMODE_DELTA == top_prefs_get_mode()) {
	/* We don't need a suffix in delta mode. */
	if(top_sprint_uinteger(buf, bufsize, i)) {
	    return true;
	}
    } else {
	if(now < prev) {
	    /* The value has decreased since the previous sample. */
	    suffix = '-';
	} else if(now > prev) {
	    suffix = '+';
	}
	
	if(-1 == snprintf(buf, bufsize, "%s%" PRIu64 "%c",
			  (i.is_negative ? "-" : ""),
			  i.value,
			  suffix)) {
	    return true;
	}
    }
 
    return false;
}


/* Return true if an error occurred. */
bool top_uinteger_format_mem_result(char *buf, size_t bufsize, uint64_t now,
				    uint64_t prev, uint64_t beg) {
    struct top_uinteger i;
    size_t len;

    i = top_uinteger_calc_result(now, prev, beg);

    if(STATMODE_DELTA == top_prefs_get_mode()) {
	/* We don't need a suffix in delta mode. */
	if(top_humanize_uinteger(buf, bufsize, i)) {
	    return true;
	}
    } else {
	if(top_humanize_uinteger(buf, bufsize - 1, i)) {
	    return true;
	}
    
	len = strlen(buf);

	if((len + 2) <= bufsize) {
	    if(now < prev) {
		buf[len] = '-';
		buf[len + 1] = '\0';
	    } else if(now > prev) {
		buf[len] = '+';
		buf[len + 1] = '\0';
	    }
	}
    }

    return false;
}
