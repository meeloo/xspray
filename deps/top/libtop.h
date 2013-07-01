/*
 * Copyright (c) 2002-2004 Apple Computer, Inc.  All rights reserved.
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

#include <mach/boolean.h>
#include <mach/host_info.h>
#include <mach/task_info.h>
#include <mach/vm_types.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/sysctl.h>

/*
 * Flags for determining whether to collect memory region information on a
 * per-process basis, used byt libtop_preg().
 */
typedef enum {
	/*
	 * Collect memory region information iff libtop_sample()'s a_reg
	 * parameter is TRUE.
	 */
	LIBTOP_PREG_default = 0,
	/* Do not collect memory region information. */
	LIBTOP_PREG_off,
	/* Always collect memory region information. */
	LIBTOP_PREG_on
} libtop_preg_t;

typedef struct libtop_i64 {
	uint64_t accumulator;
	int last_value;
} libtop_i64_t;

typedef struct libtop_i64_values {
	libtop_i64_t i64;
	uint64_t now;
	uint64_t began;
	uint64_t previous;
} libtop_i64_values_t;

/*
 * Type used for specifying a printing function that is called when an error
 * occurs.  libtop does not print a '\n' at the end of the string, so it is
 * up to the printing function to add it if desired.
 */
typedef boolean_t libtop_print_t (void *a_user_data, const char *a_format, ...);

/*
 * General sample information.
 *
 * Fields prefix meanings:
 *
 *   b_ : Value for first sample.
 *   p_ : Value for previous sample (same as b_ if p_seq is 0).
 */
typedef struct {
	/*
	 * Sample sequence number, incremented for every sample.  The first
	 * sample has a sequence number of 1.
	 */
	uint32_t		seq;

	/* Number of processes. */
	uint32_t		nprocs;

	/* CPU loads. */
	host_cpu_load_info_data_t cpu;
	host_cpu_load_info_data_t b_cpu;
	host_cpu_load_info_data_t p_cpu;

	/* Load averages for 1, 5, and 15 minutes. */
	float			loadavg[3];

	/* Start time, previous sample time, and current sample time. */
	struct timeval		time;
	struct timeval		b_time;
	struct timeval		p_time;

	/* Total number of threads. */
	uint32_t		threads;

	/* VM page size. */
	vm_size_t		pagesize;

	/* Physical memory size. */
	uint64_t		memsize;

	/* VM statistics. */
	vm_statistics_data_t	vm_stat;
	vm_statistics_data_t	b_vm_stat;
	vm_statistics_data_t	p_vm_stat;

	boolean_t		purgeable_is_valid;

	/* Swap usage */
	struct xsw_usage	xsu;
	boolean_t		xsu_is_valid;

	/* Total number of memory regions. */
	uint32_t		reg;

	/* Total shared, private, virtual sizes. */
	uint64_t		rshrd;
	uint64_t		rprvt;
	uint64_t		vsize;

	/* Total private resident memory used by frameworks. */
	uint64_t		fw_private;

	/* Total virtual memory used by frameworks. */
	uint64_t		fw_vsize;

	/* Number of frameworks. */
	uint32_t		fw_count;

	/* Code size of frameworks. */
	uint64_t		fw_code;

	/* Data size of frameworks. */
	uint64_t		fw_data;

	/* Linkedit size of frameworks. */
	uint64_t		fw_linkedit;

#define LIBTOP_STATE_MAX	7
#define LIBTOP_NSTATES		(LIBTOP_STATE_MAX + 1)
#define LIBTOP_STATE_MAXLEN	8
	int			state_breakdown[LIBTOP_NSTATES];

	/* Network statistics. */
	uint64_t		net_ipackets;
	uint64_t		b_net_ipackets;
	uint64_t		p_net_ipackets;

	uint64_t		net_opackets;
	uint64_t		b_net_opackets;
	uint64_t		p_net_opackets;

	uint64_t		net_ibytes;
	uint64_t		b_net_ibytes;
	uint64_t		p_net_ibytes;

	uint64_t		net_obytes;
	uint64_t		b_net_obytes;
	uint64_t		p_net_obytes;

	/* Disk statistics. */
	uint64_t		disk_rops;
	uint64_t		b_disk_rops;
	uint64_t		p_disk_rops;

	uint64_t		disk_wops;
	uint64_t		b_disk_wops;
	uint64_t		p_disk_wops;

	uint64_t		disk_rbytes;
	uint64_t		b_disk_rbytes;
	uint64_t		p_disk_rbytes;

	uint64_t		disk_wbytes;
	uint64_t		b_disk_wbytes;
	uint64_t		p_disk_wbytes;

	uint64_t		pages_stolen;
} libtop_tsamp_t;

/*
 * Process sample information.
 *
 * Fields prefix meanings:
 *
 *   b_ : Value for first sample.
 *   p_ : Value for previous sample (invalid if p_seq is 0).
 */
typedef struct libtop_psamp_s libtop_psamp_t;
struct libtop_psamp_s {
	uid_t			uid;
	pid_t			pid;
	pid_t			ppid;
	gid_t			pgrp;

	/* Memory statistics. */
	uint64_t		rsize;
	uint64_t		vsize;
	uint64_t		rprvt;
	uint64_t		vprvt;
	uint64_t		rshrd;
	uint64_t		fw_private;
	uint64_t		empty;

	uint32_t		reg;
	uint32_t		p_reg;

	uint64_t		p_rsize;
	uint64_t		p_vprvt;
	uint64_t		p_vsize;
	uint64_t		p_rprvt;
	uint64_t		p_rshrd;
	uint64_t		p_empty;

	/* Number of threads. */
	uint32_t		th;
	uint32_t		p_th;

	uint32_t		running_th;
	uint32_t		p_running_th;


	/* Number of ports. */
	uint32_t		prt;
	uint32_t		p_prt;

	/* CPU state/usage statistics. */
	int			state; /* Process state. */

	/* Total time consumed by process. */
	struct timeval		total_time;
	struct timeval		b_total_time;
	struct timeval		p_total_time;

	/* Event counters. */
	task_events_info_data_t	events;
	task_events_info_data_t	b_events;
	task_events_info_data_t	p_events;

	libtop_i64_values_t faults;
	libtop_i64_values_t pageins;
	libtop_i64_values_t cow_faults;
	libtop_i64_values_t messages_sent;
	libtop_i64_values_t messages_recv;
	libtop_i64_values_t syscalls_mach;
	libtop_i64_values_t syscalls_bsd;
	libtop_i64_values_t csw;

	uint64_t	palloc;
	uint64_t	pfree;
	uint64_t	salloc;
	uint64_t	sfree;

	uint64_t	p_palloc;
	uint64_t	p_pfree;
	uint64_t	p_salloc;
	uint64_t	p_sfree;

	/* malloc()ed '\0'-terminated string. */
	char			*command;

	/* Sequence number, used to detect defunct processes. */
	uint32_t		seq;

	/*
	 * Previous sequence number, used to detect processes that have only
	 * existed for the current sample (p_seq == 0).
	 */
	uint32_t		p_seq;

	/* time process was started */
	struct timeval		started;
       /* process cpu type */
	cpu_type_t	cputype;
    
	uint32_t	wq_nthreads;
	uint32_t	wq_run_threads;
	uint32_t	wq_blocked_threads;

	uint32_t	p_wq_nthreads;
	uint32_t	p_wq_run_threads;
	uint32_t	p_wq_blocked_threads;
};

/*
 * Initialize libtop.  If a non-NULL printing function pointer is passed in,
 * libtop will call the printing function when errors occur.
 *
 * Returns zero for success, non-zero for error.
 */
int
libtop_init(libtop_print_t *a_print, void *a_user_data);

/* Shut down libtop. */
void
libtop_fini(void);

/*
 * Take a sample.
 *
 * If a_reg is FALSE, do not calculate reg, vprvt, rprvt, or rshrd.
 *
 * If a_fw is FALSE, do not calculate fw_count, fw_code, fw_data, or
 * fw_linkedit. 
 *
 * Returns zero for success, non-zero for error.
 */
int
libtop_sample(boolean_t a_reg, boolean_t a_fw);

/*
 * Return a pointer to a structure containing the generic information collected
 * during the most recent sample.  The return value from this function can be
 * used for the duration of program execution (i.e. the return value does not
 * change between samples).
 */
const libtop_tsamp_t *
libtop_tsamp(void);

/*
 * Type for psamp comparison function.
 *
 * Arguments : (void *) : Opaque data pointer.
 *             (libtop_psamp_t *) : psamp.
 *
 * Return values : -1 : Second argument less than third argument.
 *                  0 : Second argument equal to third argument.
 *                  1 : Second argument greater than third argument.
 */
typedef int libtop_sort_t (void *, const libtop_psamp_t *,
    const libtop_psamp_t *);

/*
 * Sort processes using a_sort().  Pass a_data as the opaque data pointer to
 * a_sort().
 */
void
libtop_psort(libtop_sort_t *a_sort, void *a_data);

/*
 * Iteratively return a pointer to each process which was in the most recent
 * sample.  If libtop_psort() was called after the most recent libtop_sample()
 * call, the processes are iterated over in sorted order.  Otherwise, they are
 * iterated over in increasing pid order.
 *
 * A NULL return value indicates that there are no more processes to iterate
 * over.
 */
const libtop_psamp_t *
libtop_piterate(void);

/*
 * Set whether to collect memory region information for the process with pid
 * a_pid.
 *
 * Returns zero for success, non-zero for error.
 */
int
libtop_preg(pid_t a_pid, libtop_preg_t a_preg);

/*
 * Set the interval between framework updates.
 *
 * Returns zero for success, non-zero for error.
 */
int
libtop_set_interval(uint32_t ival);
#define LIBTOP_MAX_INTERVAL 100

/*
 * Return a pointer to a username string (truncated to the first 8 characters),
 * given a uid.  If the uid cannot be matched to a username, NULL is returned.
 */
const char *
libtop_username(uid_t a_uid);

/*
 * Return a pointer to a string representation of a process state (names of
 * states that are contained in libtop_tsamp_t's state_breakdown array).
 */
const char *
libtop_state_str(uint32_t a_state);

/* 
 * These i64 functions are special functions that operate on an accumulator
 * and work with overflowing 32-bit integers.  So if the value overflows in the kernel
 * counter, because it is a 32-bit value, they will in most cases capture the 
 * changes overtime to the value.  The assumption is that all updates are increments
 * of 0 or more (based on the deltas) so this doesn't work with values that
 * potentially go negative.
 */
  
libtop_i64_t
libtop_i64_init(uint64_t acc, int last_value);

void
libtop_i64_update(libtop_i64_t *i, int value);

uint64_t
libtop_i64_value(libtop_i64_t *i);
