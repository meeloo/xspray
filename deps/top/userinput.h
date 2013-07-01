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

#ifndef USERINPUT_H
#define USERINPUT_H

#include <stdbool.h>

struct user_input_state {
    char buf[60];
    int offset;
    /* This is called when Return is pressed. */
    void (*completion)(void *tinst, struct user_input_state *s);
	
    /* This is called to draw the current input text, and a prompt. */
    void (*draw)(void *tinst, struct user_input_state *s, WINDOW *win, int row, int column);
	
    int misc; /* A variable for misc things that each state may use. */
};

bool user_input_process(void *tinst);
void user_input_set_position(int r, int c);
void user_input_draw(void *tinst, WINDOW *win);
void user_input_set_error_state(const char *err);
void user_input_set_state(struct user_input_state *state);

#endif
