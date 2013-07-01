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
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <stdbool.h>
#include <unistd.h>

enum {
    STATMODE_ACCUM = 1,
    STATMODE_DELTA,
    STATMODE_EVENT,
    STATMODE_NON_EVENT
};

void top_prefs_init(void);

/* 
 * One of:
 * a accumulative mode
 * d delta mode
 * e event mode
 * n non-event mode
 */
bool top_prefs_set_mode(const char *mode);
int top_prefs_get_mode(void);
const char *top_prefs_get_mode_string(void);

void top_prefs_set_sleep(int seconds);
int top_prefs_get_sleep(void);

/* Take a symbolic string such as "cpu" */ 
bool top_prefs_set_sort(const char *sort);
/* Return one of the TOP_SORT enum values from above. */
int top_prefs_get_sort(void);

bool top_prefs_set_secondary_sort(const char *sort);
int top_prefs_get_secondary_sort(void);

const char *top_prefs_get_sort_string(void);
const char *top_prefs_get_secondary_sort_string(void);

/* This is used to sorting in ascending order (if flag is true). */
void top_prefs_set_ascending(bool flag);

bool top_prefs_get_ascending(void);

void top_prefs_set_frameworks(bool flag);
bool top_prefs_get_frameworks(void);

void top_prefs_set_frameworks_interval(int interval);
int top_prefs_get_frameworks_interval(void);

void top_prefs_set_user(const char *user);

char *top_prefs_get_user(void);

void top_prefs_set_user_uid(uid_t uid);
uid_t top_prefs_get_user_uid(void);

/* Returns true if the comma separated names list is invalid. */
bool top_prefs_set_stats(const char *names);
bool top_prefs_get_stats(int *total, int **array);

int top_prefs_get_samples(void);
void top_prefs_set_samples(int s);

int top_prefs_get_nprocs(void);
void top_prefs_set_nprocs(int n);

void top_prefs_set_pid(pid_t pid);
bool top_prefs_get_pid(pid_t *pidptr);

/* Returns true if the signal string is invalid. */ 
bool top_prefs_set_signal_string(char *s);
int top_prefs_get_signal(const char **sptr);

void top_prefs_set_logging_mode(bool mode);
bool top_prefs_get_logging_mode(void);

void top_prefs_set_ncols(int limit);
/* Returns true if the ncols has been set. */
bool top_prefs_get_ncols(int *limit);

void top_prefs_set_swap(bool show);
bool top_prefs_get_swap(void);

void top_prefs_set_secondary_ascending(bool flag);
bool top_prefs_get_secondary_ascending(void);

/* memory map reporting */
void top_prefs_set_mmr(bool mmr);
bool top_prefs_get_mmr(void);

#endif /*PREFERENCES_H*/
