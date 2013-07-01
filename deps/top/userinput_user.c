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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <pwd.h>
#include "userinput_user.h"
#include "preferences.h"

static void user_completion(void *tinst, struct user_input_state *s) {
    struct passwd *pwd;
    
    if(!strlen(s->buf)) {
	/* The user entered an empty string, so they don't want a user. */
	top_prefs_set_user(s->buf);
	user_input_set_state(NULL);
	return;
    }

    pwd = getpwnam(s->buf);

    if(NULL == pwd) {
     	user_input_set_error_state("invalid user");	
	endpwent();
	return;
    }

    top_prefs_set_user(s->buf);
    top_prefs_set_user_uid(pwd->pw_uid);

    endpwent();

    user_input_set_state(NULL);
}

static void user_draw(void *tinst, struct user_input_state *s, WINDOW *win,
		      int row, int column) {
    const char *curuser = top_prefs_get_user();

    char display[60];

    if(-1 == snprintf(display, sizeof(display), "user [%s]: %s\n",
		      curuser ? curuser : "", s->buf))
        return;

    mvwaddstr(win, row, column, display);
}

struct user_input_state top_user_input_user_state = {
    .offset = 0,
    .completion = user_completion,
    .draw = user_draw
};
