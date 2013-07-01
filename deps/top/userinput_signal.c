/*
 * Copyright (c) 2002, 2008, 2009 Apple Inc.  All rights reserved.
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
#include <signal.h>
#include <string.h>
#include <curses.h>
#include <errno.h>
#include "preferences.h"
#include "userinput.h"

static bool is_empty(const char *s) {

    while(*s) {
        if(!isspace(*s)) {
            return false;
	}

	++s;
    }

    return true;
}

static bool is_pid(const char *s) {
    const char *sp;

    for(sp = s; *sp; ++sp) {
	if(!isdigit(*sp)) {
	    return false;
	}
    }	
	
    /* If the string was not an empty string, then it contains digits. */
    if(sp != s) {
	return true;
    }

    return false;
}

static void reset_pid(struct user_input_state *s) {
    s->buf[0] = '\0';
    s->offset = 0;
}

static void signal_pid_completion(void *tinst, struct user_input_state *s) {
    const char *signame;
    int sig;
    int err;
    uid_t euid;
    gid_t egid;
    int saved_errno = 0;
    pid_t pid;
  
    if(is_empty(s->buf)) {
	/*
	 * Any empty buffer indicates that the user didn't want
	 * to signal the process after all.
	 */

	reset_pid(s);
	user_input_set_state(NULL);
        return;
    }

    if(!is_pid(s->buf)) {
	reset_pid(s);
	user_input_set_error_state("invalid pid");
	return;
    }
    
    pid = atoi(s->buf);

    reset_pid(s);

    sig = top_prefs_get_signal(&signame);
    
    /* Temporarily drop permissions. */
    euid = geteuid();
    egid = getegid();
    
    if(-1 == seteuid(getuid())
       || -1 == setegid(getgid())) {
	user_input_set_error_state("missing setuid bit");
	return;
    }
    
    err = kill(pid, sig);

    if(-1 == err)
	saved_errno = errno;

    if(-1 == seteuid(euid)
       || -1 == setegid(egid)) {
	user_input_set_error_state("restoring setuid bit");
	return;
    }

    switch(saved_errno) {
    case EINVAL:
	user_input_set_error_state("invalid signal");
	return;

    case ESRCH:
	user_input_set_error_state("invalid pid");
	return;

    case EPERM:
	user_input_set_error_state("permission error signaling");
	return;
    }

    user_input_set_state(NULL);
}

static void signal_pid_draw(void *tinst, struct user_input_state *s, 
			    WINDOW *win, int row, int column) {
    char display[60];

    if(-1 == snprintf(display, sizeof(display), "pid: %s", s->buf)) {
	user_input_set_error_state("string input too long!");
	return;
    }

    mvwaddstr(win, row, column, display);
}

struct user_input_state top_user_input_signal_pid_state = {
    .offset = 0,
    .completion = signal_pid_completion,
    .draw = signal_pid_draw
};

static void signal_completion(void *tinst, struct user_input_state *s) {

    if(!strlen(s->buf)) {
	/* Use the current default. */
	user_input_set_state(&top_user_input_signal_pid_state);
	return;
    }

    if(top_prefs_set_signal_string(s->buf)) {
	user_input_set_error_state("invalid signal name");
	return;
    }

    user_input_set_state(&top_user_input_signal_pid_state); 
}

static void signal_draw(void *tinst, struct user_input_state *s, WINDOW *win,
		       int row, int column) {
    char display[60];
    const char *signame;
    
    (void)top_prefs_get_signal(&signame);

    if(-1 == snprintf(display, sizeof(display), "signal [%s]: %s",
		      signame, s->buf)) {
	user_input_set_error_state("string input too long!");
	return;
    }

    mvwaddstr(win, row, column, display);
}

struct user_input_state top_user_input_signal_state = {
    .offset = 0,
    .completion = signal_completion,
    .draw = signal_draw
};
