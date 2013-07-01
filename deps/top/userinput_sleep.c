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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <curses.h>
#include "preferences.h"
#include "userinput.h"

static void sleep_completion(void *tinst, struct user_input_state *s) {
    int delay = 0;
    char *p;
    bool got_digit = false;

    if(!strlen(s->buf)) {
	/* Use the current default. */
	user_input_set_state(NULL);
	return;
    }

    for(p = s->buf; *p; ++p) {
	if(isdigit(*p)) {
	    got_digit = true;
	} else {
	    user_input_set_error_state("not a valid sleep delay");
	    return;
	}
    }    

    if(!strlen(s->buf) || !got_digit) {
	user_input_set_error_state("not a valid sleep delay");
	return;
    }

    delay = atoi(s->buf);

    if(delay < 0) {
	user_input_set_error_state("delay is negative");
	return;
    }

    top_prefs_set_sleep(delay);

    user_input_set_state(NULL);
}

static void sleep_draw(void *tinst, struct user_input_state *s, WINDOW *win,
		       int row, int column) {
    char display[60];

    if(-1 == snprintf(display, sizeof(display), "update interval[%d]: %s\n",
		      top_prefs_get_sleep(), s->buf))
	return;

    mvwaddstr(win, row, column, display);
}

struct user_input_state top_user_input_sleep_state = {
    .offset = 0,
    .completion = sleep_completion,
    .draw = sleep_draw
};
