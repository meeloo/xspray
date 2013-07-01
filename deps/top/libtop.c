/*
 * Copyright (c) 2002-2004, 2008, 2009 Apple Inc.  All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <mach/bootstrap.h>
#include <mach/host_priv.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/mach_port.h>
#include <mach/mach_vm.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/processor_set.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/shared_region.h>
#include <mach/vm_map.h>

#define IOKIT 1 /* For io_name_t in device/device_types.h. */
#include <device/device_types.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>

#include <libproc.h>

#include <fcntl.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <pwd.h>

#include <sys/resource.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <net/if_types.h>
#include <ifaddrs.h>

#define LIBTOP_DBG
#ifndef LIBTOP_DBG
/* Disable assertions. */
#ifndef NDEBUG
#define NDEBUG
#endif
#endif
#include <assert.h>

#include "libtop.h"
#include "rb.h"

/*
 * Process info.
 */
typedef struct libtop_pinfo_s libtop_pinfo_t;
struct libtop_pinfo_s {
	/* Sample data that are exposed to the library user. */
	libtop_psamp_t		psamp;

	/* Linkage for pid-ordered tree. */
	rb_node(libtop_pinfo_t)	pnode;

	/* Linkage for sorted tree. */
	rb_node(libtop_pinfo_t)	snode;

	int			flag; /* Set, but not used. */

	/* Manual override for memory region reporting. */
	libtop_preg_t		preg;

	/* TRUE if the globally shared text and data segments are mapped in. */
	boolean_t		split;
};

/* Memory object info. */
typedef struct libtop_oinfo_s libtop_oinfo_t;
struct libtop_oinfo_s {
	/*
	 * pinfo structure that was most recently used to access this
	 * structure.
	 */
	libtop_pinfo_t	*pinfo;

	/* Memory object ID. */
	int		obj_id;

	/* Memory object size, in pages. */
	int		size;

	/* SM_PRIVATE, SM_SHARED, SM_PRIVATE_ALIASED, SM_COW, ... */
	int		share_type;

	/* Number of pages resident in memory. */
	int		resident_page_count;

	/* Number of references to memory object. */
	int		ref_count;

	/* Number of references to memory object that pinfo holds. */
	int		proc_ref_count;

	/*
	 * Rollback fields.  These are used to store old values that need to be
	 * rolled back to their previous values if an object is referenced more
	 * than once by a process.
	 *
	 * The reason rolling back is necessary has to do with the fact that
	 * memory usage statistics are tallied on the fly, but this can only be
	 * accurately done for each memory region a process refers to once all
	 * of the references have been encountered.  Since the code
	 * optimistically updates the statistics, changes have to be backed out
	 * when another reference to the same memory region is encountered.
	 */
	int			rb_share_type; /* SM_EMPTY == "no rollback" */
	uint64_t	rb_aliased;
	uint64_t	rb_vprvt;
	uint64_t	rb_rshrd;
};

static boolean_t ignore_PPP;

/* Sample data that are exposed to the library user. */
static libtop_tsamp_t tsamp;

/* Function pointer that points to an abstract printing function. */
static libtop_print_t *libtop_print;
static void *libtop_user_data;

/* Temporary storage for sorting function and opaque data pointer. */
static libtop_sort_t *libtop_sort;
static void *libtop_sort_data;

/* Mach port, used for various Mach calls. */
static mach_port_t libtop_port;

/* Buffer that is large enough to hold the entire argument area of a process. */
static char *libtop_arg;
static int libtop_argmax;

static mach_port_t libtop_master_port;

static uint32_t interval;

/*
 * Memory object hash table and list.  For each sample, a hash table of memory
 * objects is created, and it is used to determine various per-process memory
 * statistics, as well as the total number of memory objects.  Rather than
 * allocating and freeing oinfo structures for every sample, previously
 * allocated structures are linked into a list, and objects in the list are used
 * in preference to allocation.
 */
static CFMutableDictionaryRef libtop_oinfo_hash;

/* Tree of all pinfo's, always ordered by -pid. */
static rb_tree(libtop_pinfo_t) libtop_ptree;
/*
 * Transient tree of all pinfo's, created for each sample according to a
 * sorting function.
 */
static rb_tree(libtop_pinfo_t) libtop_stree;

/* TRUE if the most recent sample is sorted. */
static boolean_t libtop_sorted;

/* Pointer to the most recently seen pinfo structure in libtop_piterate(). */
static libtop_pinfo_t *libtop_piter;

/* Cache of uid->username translations. */
static CFMutableDictionaryRef libtop_uhash;

#define	TIME_VALUE_TO_TIMEVAL(a, r) do {				\
	(r)->tv_sec = (a)->seconds;					\
	(r)->tv_usec = (a)->microseconds;				\
} while (0)

enum libtop_status {
	LIBTOP_NO_ERR = 0,
	LIBTOP_ERR_INVALID = 1, /* An invalid task. */
	LIBTOP_ERR_ALLOC  /* An allocation error. */
};

typedef enum libtop_status libtop_status_t; 

/* Function prototypes. */
static boolean_t libtop_p_print(void *user_data, const char *format, ...);
static int libtop_p_mach_state_order(int state, long sleep_time);
static int libtop_p_load_get(host_info_t r_load);
static int libtop_p_loadavg_update(void);
static bool in_shared_region(mach_vm_address_t addr, cpu_type_t type);
static void libtop_p_fw_scan(task_t task, mach_vm_address_t region_base,
	mach_vm_size_t region_size);
static void libtop_p_fw_sample(boolean_t fw);
static int libtop_p_vm_sample(void);
static void libtop_p_networks_sample(void);
static int libtop_p_disks_sample(void);
static int libtop_p_proc_table_read(boolean_t reg);
static libtop_status_t libtop_p_cputype(pid_t pid, cpu_type_t *cputype);
static mach_vm_size_t libtop_p_shreg_size(cpu_type_t);
static libtop_status_t libtop_p_task_update(task_t task, boolean_t reg);
static libtop_status_t libtop_p_proc_command(libtop_pinfo_t *pinfo,
	struct kinfo_proc *kinfo);
static void libtop_p_pinsert(libtop_pinfo_t *pinfo);
static void libtop_p_premove(libtop_pinfo_t *pinfo);
static void libtop_p_destroy_pinfo(libtop_pinfo_t *pinfo);
static libtop_pinfo_t* libtop_p_psearch(pid_t pid);
static int libtop_p_pinfo_pid_comp(libtop_pinfo_t *a, libtop_pinfo_t *b);
static int libtop_p_pinfo_comp(libtop_pinfo_t *a, libtop_pinfo_t *b);
static libtop_oinfo_t* libtop_p_oinfo_insert(int obj_id, int share_type,
	int resident_page_count, int ref_count, int size,
	libtop_pinfo_t *pinfo);
static int libtop_p_wq_update(libtop_pinfo_t *pinfo);
static void libtop_i64_test(void);
static void update_pages_stolen(libtop_tsamp_t *tsamp);


/* CFDictionary callbacks */
static void
simpleFree(CFAllocatorRef allocator, const void *value)
{
	free((void *)value);
}

static const void *
stringRetain(CFAllocatorRef allocator, const void *value)
{
	return strdup(value);
}

static Boolean
stringEqual(const void *value1, const void *value2)
{
	return strcmp(value1, value2) == 0;
}

int
libtop_init(libtop_print_t *print, void *user_data)
{
	//libtop_i64_test();
	
	if (print != NULL) {
		libtop_print = print;
		libtop_user_data = user_data;
	} else {
		/* Use a noop printing function. */
		libtop_print = libtop_p_print;
		libtop_user_data = NULL;
	}

	tsamp.seq = 0;
	interval = 1;
	libtop_port = mach_host_self();
	host_page_size(libtop_port, &tsamp.pagesize);

	/* Get the physical memory size of the system. */
	{
		int mib[2];
		size_t size;

		mib[0] = CTL_HW;
		mib[1] = HW_MEMSIZE;

		size = sizeof(tsamp.memsize);
		if (sysctl(mib, 2, &tsamp.memsize, &size, NULL, 0) == -1) {
			libtop_print(libtop_user_data, "%s(): Error in sysctl(): %s",
			    __FUNCTION__, strerror(errno));
			return -1;
		}
	}

	update_pages_stolen(&tsamp);

	/* Initialize the pinfo tree. */
	rb_tree_new(&libtop_ptree, pnode);

	/* Initialized the user hash. */
	CFDictionaryValueCallBacks stringValueCallBacks = { 0, stringRetain, simpleFree, NULL, stringEqual };
	libtop_uhash = CFDictionaryCreateMutable(NULL, 0, NULL, &stringValueCallBacks);

	/* Initialize the memory object hash table and spares ring. */
	CFDictionaryValueCallBacks oinfoValueCallBacks = { 0, NULL, simpleFree, NULL, NULL };
	libtop_oinfo_hash = CFDictionaryCreateMutable(NULL, 0, NULL, &oinfoValueCallBacks);

	/*
	 * Allocate a buffer that is large enough to hold the maximum arguments
	 * to execve().  This is used when getting the arguments to programs.
	 */
	{
		int	mib[2];
		size_t	size;

		mib[0] = CTL_KERN;
		mib[1] = KERN_ARGMAX;

		size = sizeof(libtop_argmax);
		if (sysctl(mib, 2, &libtop_argmax, &size, NULL, 0) == -1) {
			libtop_print(libtop_user_data, "%s(): Error in sysctl(): %s",
			    __FUNCTION__, strerror(errno));
			return -1;
		}

		libtop_arg = (char *)malloc(libtop_argmax);
		if (libtop_arg == NULL) return -1;
	}

	/*
	 * Get ports and services for drive statistics.
	 */

	if (IOMasterPort(bootstrap_port, &libtop_master_port)) {
		libtop_print(libtop_user_data, "Error in IOMasterPort()");
		return -1;
	}

	/* Initialize the load statistics. */
	if (libtop_p_load_get((host_info_t)&tsamp.b_cpu)) return -1;

	tsamp.p_cpu = tsamp.b_cpu;
	tsamp.cpu = tsamp.b_cpu;

	/* Initialize the time. */
	gettimeofday(&tsamp.b_time, NULL);
	tsamp.p_time = tsamp.b_time;
	tsamp.time = tsamp.b_time;

	ignore_PPP = FALSE;
	return 0;
}

void
libtop_fini(void)
{
	libtop_pinfo_t *pinfo, *ppinfo;

	/* Deallocate the arg string. */
	free(libtop_arg);

	/* Clean up the oinfo structures. */
	CFRelease(libtop_oinfo_hash);

	/* Clean up the pinfo structures. */
	rb_first(&libtop_ptree, pnode, pinfo);
	for (;
	     pinfo != rb_tree_nil(&libtop_ptree);
	     pinfo = ppinfo) {
		rb_next(&libtop_ptree, pinfo, libtop_pinfo_t, pnode, ppinfo);

		/* This removes the pinfo from the tree, and frees pinfo and its data. */
		libtop_p_destroy_pinfo(pinfo);
	}

	/* Clean up the uid->username translation cache. */
	CFRelease(libtop_uhash);
}

/*
 * Set the interval between framework updates.
 */
int
libtop_set_interval(uint32_t ival)
{
	if (ival < 0 || ival > LIBTOP_MAX_INTERVAL) {
		return -1;
	}
	interval = ival;
	return 0;
}
	
/* Take a sample. */
int
libtop_sample(boolean_t reg, boolean_t fw)
{
	int res = 0;
	
	/* Increment the sample sequence number. */
	tsamp.seq++;

	/*
	 * Make a note that the results haven't been sorted (reset by
	 * libtop_psort()).
	 */
	libtop_sorted = FALSE;
	libtop_piter = NULL;

	/* Clear state breakdown. */
	memset(tsamp.state_breakdown, 0, sizeof(tsamp.state_breakdown));

	/* Get time. */
	if (tsamp.seq != 1) {
		tsamp.p_time = tsamp.time;
		res = gettimeofday(&tsamp.time, NULL);
	}

	if (res == 0) res = libtop_p_proc_table_read(reg);
	if (res == 0) res = libtop_p_loadavg_update();
	   
	/* Get CPU usage counters. */
	tsamp.p_cpu = tsamp.cpu;
	if (res == 0) libtop_p_load_get((host_info_t)&tsamp.cpu);

	if (res == 0) libtop_p_fw_sample(fw);
	if (res == 0) libtop_p_vm_sample();
	if (res == 0) libtop_p_networks_sample();
	if (res == 0) libtop_p_disks_sample();
        
	return res;
}

/* Return a pointer to the structure that contains libtop-wide data. */
const libtop_tsamp_t *
libtop_tsamp(void)
{
	return &tsamp;
}

/*
 * Given a tree of pinfo structures, create another tree that is sorted
 * according to sort().
 */
void
libtop_psort(libtop_sort_t *sort, void *data)
{
	libtop_pinfo_t	*pinfo, *ppinfo;

	assert(tsamp.seq != 0);

	/* Reset the iteration pointer. */
	libtop_piter = NULL;

	/* Initialize the sorted tree. */
	rb_tree_new(&libtop_stree, snode);

	/* Note that the results are sorted. */
	libtop_sorted = TRUE;

	/*
	 * Set the sorting function and opaque data in preparation for building
	 * the sorted tree.
	 */
	libtop_sort = sort;
	libtop_sort_data = data;

	/*
	 * Iterate through ptree and insert the pinfo's into a sorted tree.
	 * At the same time, prune pinfo's that were associated with processes
	 * that were not found during the most recent sample.
	 */
	tsamp.nprocs = 0;
	rb_first(&libtop_ptree, pnode, pinfo);
	for (;
	     pinfo != rb_tree_nil(&libtop_ptree);
	     pinfo = ppinfo) {
		/*
		 * Get the next pinfo before potentially removing this one from
		 * the tree.
		 */
		rb_next(&libtop_ptree, pinfo, libtop_pinfo_t, pnode, ppinfo);

		if (pinfo->psamp.seq == tsamp.seq) {
			/* Insert the pinfo into the sorted tree. */
			rb_node_new(&libtop_stree, pinfo, snode);
			rb_insert(&libtop_stree, pinfo, libtop_p_pinfo_comp,
			    libtop_pinfo_t, snode);

			tsamp.nprocs++;
		} else {
			/* The associated process has gone away. */
			libtop_p_destroy_pinfo(pinfo);
		}
	}
}

/*
 * Iteratively return a pointer to each process that was in the most recent
 * sample.  The order depends on if/how libtop_psort() was called.
 */
const libtop_psamp_t *
libtop_piterate(void)
{
	assert(tsamp.seq != 0);

	if (libtop_sorted) {
		/* Use the order set by libtop_psort(). */
		if (libtop_piter == NULL) {
			rb_first(&libtop_stree, snode, libtop_piter);
		} else {
			rb_next(&libtop_stree, libtop_piter, libtop_pinfo_t,
			    snode, libtop_piter);
		}
		if (libtop_piter == rb_tree_nil(&libtop_stree)) {
			libtop_piter = NULL;
		}
	} else {
		boolean_t	dead;

		/*
		 * Return results in ascending pid order.  Since dead processes
		 * weren't cleaned out by libtop_psamp(), take care to do so
		 * here on the fly.
		 */
		if (libtop_piter == NULL) {
			rb_first(&libtop_ptree, pnode, libtop_piter);
		} else {
			rb_next(&libtop_ptree, libtop_piter, libtop_pinfo_t,
			    pnode, libtop_piter);
		}

		do {
			dead = FALSE;

			if (libtop_piter == rb_tree_nil(&libtop_ptree)) {
				/* No more tree nodes. */
				libtop_piter = NULL;
				break;
			}
			if (libtop_piter->psamp.seq != tsamp.seq) {
				libtop_pinfo_t	*pinfo;

				/*
				 * Dead process.  Get the next pinfo tree node
				 * before removing this one.
				 */
				pinfo = libtop_piter;
				rb_next(&libtop_ptree, libtop_piter,
				    libtop_pinfo_t, pnode, libtop_piter);

				libtop_p_destroy_pinfo(pinfo);

				dead = TRUE;
			}
		} while (dead);
	}

	return &libtop_piter->psamp;
}

/*
 * Set whether to collect memory region information for the process with pid
 * pid.
 */
int
libtop_preg(pid_t pid, libtop_preg_t preg)
{
	libtop_pinfo_t	*pinfo;

	pinfo = libtop_p_psearch(pid);
	if (pinfo == NULL) return -1;
	pinfo->preg = preg;

	return 0;
}

/* Return a pointer to the username string associated with uid. */
const char *
libtop_username(uid_t uid)
{
	const void *k = (const void *)(uintptr_t)uid;

	if (!CFDictionaryContainsKey(libtop_uhash, k)) {
		struct passwd *pwd = getpwuid(uid);
		if (pwd == NULL)
			return NULL;
		CFDictionarySetValue(libtop_uhash, k, pwd->pw_name);
	}

	return CFDictionaryGetValue(libtop_uhash, k);
}

/* Return a pointer to a string representation of a process state. */
const char *
libtop_state_str(uint32_t state)
{
	const char *strings[] = {
		"zombie",
#define LIBTOP_STATE_ZOMBIE	0
		"running",
#define LIBTOP_STATE_RUN	1
		"stuck",
#define LIBTOP_STATE_STUCK	2
		"sleeping",
#define LIBTOP_STATE_SLEEP	3
		"idle",
#define LIBTOP_STATE_IDLE	4
		"stopped",
#define LIBTOP_STATE_STOP	5
		"halted",
#define LIBTOP_STATE_HALT	6
		"unknown"
#define LIBTOP_STATE_UNKNOWN	7
	};

	assert(LIBTOP_NSTATES == sizeof(strings) / sizeof(char *));
	assert(state <= LIBTOP_STATE_MAX);
	assert(LIBTOP_STATE_MAXLEN >= 8); /* "sleeping" */

	return strings[state];
}

/*
 * Noop printing function, used when the user doesn't supply a printing
 * function.
 */
static boolean_t
libtop_p_print(void *user_data, const char *format, ...)
{
	/* Do nothing. */
	return 0;
}

/* Translate a mach state to a state in the state breakdown array. */
static int
libtop_p_mach_state_order(int state, long sleeptime)
{
	switch (state) {
		case TH_STATE_RUNNING:
			return LIBTOP_STATE_RUN;
		case TH_STATE_UNINTERRUPTIBLE:
			return LIBTOP_STATE_STUCK;
		case TH_STATE_STOPPED:
			return LIBTOP_STATE_STOP;
		case TH_STATE_HALTED:
			return LIBTOP_STATE_HALT;
		case TH_STATE_WAITING:
			return (sleeptime > 0) ? LIBTOP_STATE_IDLE : LIBTOP_STATE_SLEEP;
		default:
			return LIBTOP_STATE_UNKNOWN;
	}
}

/* Get CPU load. */
static int
libtop_p_load_get(host_info_t r_load)
{
	kern_return_t kr;
	mach_msg_type_number_t count;

	count = HOST_CPU_LOAD_INFO_COUNT;
	kr = host_statistics(libtop_port, HOST_CPU_LOAD_INFO, r_load, &count);
	if (kr != KERN_SUCCESS) {
		libtop_print(libtop_user_data, "Error in host_statistics(): %s",
		    mach_error_string(kr));
		return -1;
	}

	return 0;
}

/* Update load averages. */
static int
libtop_p_loadavg_update(void)
{
	double avg[3];

	if (getloadavg(avg, sizeof(avg)) < 0) {
		libtop_print(libtop_user_data,  "Error in getloadavg(): %s",
			strerror(errno));
		return -1;
	}

	tsamp.loadavg[0] = avg[0];
	tsamp.loadavg[1] = avg[1];
	tsamp.loadavg[2] = avg[2];

	return 0;
}

/*
 * Test whether the virtual address is within the architecture's shared region.
 */
static bool
in_shared_region(mach_vm_address_t addr, cpu_type_t type) {
	mach_vm_address_t base = 0, size = 0;

	switch(type) {
		case CPU_TYPE_ARM:
			base = SHARED_REGION_BASE_ARM;
			size = SHARED_REGION_SIZE_ARM;
		break;

		case CPU_TYPE_X86_64:
			base = SHARED_REGION_BASE_X86_64;
			size = SHARED_REGION_SIZE_X86_64;
		break;

		case CPU_TYPE_I386:
			base = SHARED_REGION_BASE_I386;
			size = SHARED_REGION_SIZE_I386;
		break;

		case CPU_TYPE_POWERPC:
			base = SHARED_REGION_BASE_PPC;
			size = SHARED_REGION_SIZE_PPC;
		break;

		case CPU_TYPE_POWERPC64:
			base = SHARED_REGION_BASE_PPC64;
			size = SHARED_REGION_SIZE_PPC64;
		break;
	
		default: {
			int t = type;

			fprintf(stderr, "unknown CPU type: 0x%x\n", t);
			abort();
		}
		break;
	}


	return(addr >= base && addr < (base + size));
}

/* Iterate through a given region of memory, adding up the various
   submap regions found therein.  Modifies tsamp. */
static void
libtop_p_fw_scan(task_t task, mach_vm_address_t region_base, mach_vm_size_t region_size) {
	mach_vm_size_t	vsize = 0;
	mach_vm_size_t	code = 0;
	mach_vm_size_t	data = 0;
	mach_vm_size_t	linkedit = 0;
	mach_vm_size_t	pagesize = tsamp.pagesize;
	
	mach_vm_address_t	addr;
	mach_vm_size_t		size;

	
	for (addr = region_base; addr < (region_base + region_size); addr += size) {
		kern_return_t kr;
		uint32_t depth = 1;
		vm_region_submap_info_data_64_t sinfo;
		mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

		// Get the next submap in the specified region of memory
		kr = mach_vm_region_recurse(task, &addr, &size, &depth,
									(vm_region_recurse_info_t)&sinfo, &count);
		if (kr != KERN_SUCCESS) break;

		/* 
		 * There should be no reason to test if addr is in a shared 
		 * region, because the for loop limits the region space to the
		 * passed SHARED_REGION_BASE and SHARED_REGION_SIZE.
		 */

		vsize += size;
		
		switch (sinfo.share_mode) {
			case SM_SHARED:
			case SM_COW:
			case SM_TRUESHARED:
				if (sinfo.max_protection & VM_PROT_EXECUTE) {
					// CODE
					code += sinfo.pages_resident;
					tsamp.fw_count++;
				} else if (sinfo.max_protection & VM_PROT_WRITE) {
					// DATA
					data += sinfo.pages_resident;
				} else {
					// LINKEDIT
					linkedit += sinfo.pages_resident;
				}
				break;
		}
	}

	tsamp.fw_vsize += vsize;
	tsamp.fw_code += code * pagesize;
	tsamp.fw_data += data * pagesize;
	tsamp.fw_linkedit += linkedit * pagesize;
}

/* Sample framework memory statistics (if fw is TRUE). */
static void
libtop_p_fw_sample(boolean_t fw)
{
	if (!fw) return;
	if ((interval != 1) && ((tsamp.seq % interval) != 1)) return;

	tsamp.fw_count = 0;
	tsamp.fw_code = 0;
	tsamp.fw_data = 0;
	tsamp.fw_linkedit = 0;
	tsamp.fw_vsize = 0;
	tsamp.fw_private = 0;

#ifdef	__arm__
	libtop_p_fw_scan(mach_task_self(), SHARED_REGION_BASE_ARM, SHARED_REGION_SIZE_ARM);
#else
	libtop_p_fw_scan(mach_task_self(), SHARED_REGION_BASE_PPC, SHARED_REGION_SIZE_PPC);
	libtop_p_fw_scan(mach_task_self(), SHARED_REGION_BASE_PPC64, SHARED_REGION_SIZE_PPC64);

	assert(SHARED_REGION_BASE_PPC <= SHARED_REGION_BASE_I386);
	assert(SHARED_REGION_SIZE_PPC >= SHARED_REGION_SIZE_I386);
	// Skip: i386 region is a subset of PPC region.
	//libtop_p_fw_scan(mach_task_self(), SHARED_REGION_BASE_I386, SHARED_REGION_SIZE_I386);
	
	assert(SHARED_REGION_BASE_PPC64 <= SHARED_REGION_BASE_X86_64);
	assert(SHARED_REGION_SIZE_PPC64 >= SHARED_REGION_SIZE_X86_64);
	// Skip: x86_64 region is a subset of ppc64 region.
	//libtop_p_fw_scan(mach_task_self(), SHARED_REGION_BASE_X86_64, SHARED_REGION_SIZE_X86_64);
#endif

	// Iterate through all processes, collecting their individual fw stats
	libtop_piter = NULL;
	while (libtop_piterate()) {
		tsamp.fw_private += libtop_piter->psamp.fw_private;
	}
}

static int
libtop_tsamp_update_swap_usage(libtop_tsamp_t* tsamp) {
	int mib[2] = { CTL_VM, VM_SWAPUSAGE };
	int miblen = 2;
	size_t len = sizeof(tsamp->xsu);
	int res = sysctl(mib, miblen, &tsamp->xsu, &len, NULL, 0);
	tsamp->xsu_is_valid = (res == 0);
	return res;
}

/* This is for <rdar://problem/6410098>. */
static uint64_t
round_down_wired(uint64_t value) {
	return (value & ~((128 * 1024 * 1024ULL) - 1));
}

/* This is for <rdar://problem/6410098>. */
static void
update_pages_stolen(libtop_tsamp_t *tsamp) {
	static int mib_reserved[CTL_MAXNAME];
	static int mib_unusable[CTL_MAXNAME];
	static int mib_other[CTL_MAXNAME];
	static size_t mib_reserved_len = 0;
	static size_t mib_unusable_len = 0;
	static size_t mib_other_len = 0;
	int r;
	
	tsamp->pages_stolen = 0;

	/* This can be used for testing: */
	//tsamp->pages_stolen = (256 * 1024 * 1024ULL) / tsamp->pagesize;

	if(0 == mib_reserved_len) {
		mib_reserved_len = CTL_MAXNAME;
		
		r = sysctlnametomib("machdep.memmap.Reserved", mib_reserved,
				    &mib_reserved_len);

		if(-1 == r) {
			mib_reserved_len = 0;
			return;
		}

		mib_unusable_len = CTL_MAXNAME;
	
		r = sysctlnametomib("machdep.memmap.Unusable", mib_unusable,
				    &mib_unusable_len);	

		if(-1 == r) {
			mib_reserved_len = 0;
			return;
		}


		mib_other_len = CTL_MAXNAME;
		
		r = sysctlnametomib("machdep.memmap.Other", mib_other,
				    &mib_other_len);

		if(-1 == r) {
			mib_reserved_len = 0;
			return;
		}
	}		

	if(mib_reserved_len > 0 && mib_unusable_len > 0 && mib_other_len > 0) {
		uint64_t reserved = 0, unusable = 0, other = 0;
		size_t reserved_len;
		size_t unusable_len;
		size_t other_len;
		
		reserved_len = sizeof(reserved);
		unusable_len = sizeof(unusable);
		other_len = sizeof(other);

		/* These are all declared as QUAD/uint64_t sysctls in the kernel. */
	
		if(-1 == sysctl(mib_reserved, mib_reserved_len, &reserved, 
				&reserved_len, NULL, 0)) {
			return;
		}

		if(-1 == sysctl(mib_unusable, mib_unusable_len, &unusable,
				&unusable_len, NULL, 0)) {
			return;
		}

		if(-1 == sysctl(mib_other, mib_other_len, &other,
				&other_len, NULL, 0)) {
			return;
		}

		if(reserved_len == sizeof(reserved) 
		   && unusable_len == sizeof(unusable) 
		   && other_len == sizeof(other)) {
			uint64_t stolen = reserved + unusable + other;	
			uint64_t mb128 = 128 * 1024 * 1024ULL;

			if(stolen >= mb128) {
				tsamp->pages_stolen = round_down_wired(stolen) / tsamp->pagesize;
			}
		}
	}
}



static int
libtop_tsamp_update_vm_stats(libtop_tsamp_t* tsamp) {
	kern_return_t kr;
	tsamp->p_vm_stat = tsamp->vm_stat;
	
	mach_msg_type_number_t count = sizeof(tsamp->vm_stat) / sizeof(natural_t);
	kr = host_statistics(libtop_port, HOST_VM_INFO, (host_info_t)&tsamp->vm_stat, &count);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	if (tsamp->pages_stolen > 0) {
		tsamp->vm_stat.wire_count += tsamp->pages_stolen;
	}
	
	// Check whether we got purgeable memory statistics
	tsamp->purgeable_is_valid = (count == (sizeof(tsamp->vm_stat)/sizeof(natural_t)));
	if (!tsamp->purgeable_is_valid) {
		tsamp->vm_stat.purgeable_count = 0;
		tsamp->vm_stat.purges = 0;
	}

	if (tsamp->seq == 1) {
		tsamp->p_vm_stat = tsamp->vm_stat;
		tsamp->b_vm_stat = tsamp->vm_stat;
	}
	
	return kr;
}

static void
sum_rshrd(const void *key, const void *value, void *context)
{
	libtop_oinfo_t *oinfo = (libtop_oinfo_t *)value;
	mach_vm_size_t *rshrd = (mach_vm_size_t *)context;

	switch (oinfo->share_type) {
	case SM_SHARED:
	case SM_COW:
		*rshrd += oinfo->resident_page_count;
		break;
	}
}

/* Sample general VM statistics. */
static int
libtop_p_vm_sample(void)
{
	/* Get VM statistics. */
	libtop_tsamp_update_vm_stats(&tsamp);

	/* Get swap usage */
	libtop_tsamp_update_swap_usage(&tsamp);

	/*
	 * Iterate through the oinfo hash table and add up the collective size
	 * of the shared objects.
	 */

	mach_vm_size_t reg = 0;
	mach_vm_size_t rprvt = 0;
	mach_vm_size_t rshrd = 0;
	mach_vm_size_t vsize = 0;
	
	CFDictionaryApplyFunction(libtop_oinfo_hash, sum_rshrd, &rshrd);

	/* Iterate through all processes, collecting their individual vm stats */
	libtop_piter = NULL;
	while (libtop_piterate()) {
		reg += libtop_piter->psamp.reg;
		rprvt += libtop_piter->psamp.rprvt;
		vsize += libtop_piter->psamp.vsize;
	}

	tsamp.reg = reg;
	tsamp.rprvt = rprvt;
	tsamp.rshrd = rshrd * tsamp.pagesize;
	tsamp.vsize = vsize;
	
	return 0;
}

/*
 * Sample network usage.
 *
 * The algorithm used does not deal with the following condition, which can
 * cause the statistics to be invalid:
 *
 *  Interfaces are dynamic -- they can appear and disappear at any time.
 *  There is no way to get statistics on an interface that has disappeared, so
 *  it isn't possible to determine the amount of data transfer between the
 *  previous sample and when the interface went away.
 *
 *  Due to this problem, if an interface disappears, it is possible for the
 *  current sample values to be lower than those of the beginning or previous
 *  samples.
 */
static void
libtop_p_networks_sample(void)
{
	short network_layer;
	short link_layer;
 	int mib[6];
     	char *buf = NULL, *lim, *next;
	size_t len;
	struct if_msghdr *ifm;


	tsamp.p_net_ipackets = tsamp.net_ipackets;
	tsamp.p_net_opackets = tsamp.net_opackets;
	tsamp.p_net_ibytes = tsamp.net_ibytes;
	tsamp.p_net_obytes = tsamp.net_obytes;

	tsamp.net_ipackets = 0;
	tsamp.net_opackets = 0;
	tsamp.net_ibytes = 0;
	tsamp.net_obytes = 0;

	mib[0]	= CTL_NET;			// networking subsystem
	mib[1]	= PF_ROUTE;			// type of information
	mib[2]	= 0;				// protocol (IPPROTO_xxx)
	mib[3]	= 0;				// address family
	mib[4]	= NET_RT_IFLIST2;	// operation
	mib[5]	= 0;
	if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) return;
	if ((buf = malloc(len)) == NULL) {
		printf("malloc failed\n");
		exit(1);
	}
	if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
		if (buf) free(buf);
		return;
	}

	lim = buf + len;
	for (next = buf; next < lim; ) {
		network_layer = link_layer = 0;
	        ifm = (struct if_msghdr *)next;
		next += ifm->ifm_msglen;

	        if (ifm->ifm_type == RTM_IFINFO2) {
			struct if_msghdr2 	*if2m = (struct if_msghdr2 *)ifm;

			if(if2m->ifm_data.ifi_type==IFT_ETHER)  /* If we've seen any ethernet traffic, */
				ignore_PPP=TRUE; 		/* ignore any PPP traffic (PPPoE or VPN) */


			if(/*We want to count them all in top.*/ 1 
				|| ((if2m->ifm_data.ifi_type!=IFT_LOOP)   /* do not count loopback traffic */
			   && !(ignore_PPP && if2m->ifm_data.ifi_type==IFT_PPP))) { /* or VPN/PPPoE */
				tsamp.net_opackets += if2m->ifm_data.ifi_opackets;
				tsamp.net_ipackets += if2m->ifm_data.ifi_ipackets;
				tsamp.net_obytes   += if2m->ifm_data.ifi_obytes;
				tsamp.net_ibytes   += if2m->ifm_data.ifi_ibytes;
			}
		} 
	}

	if (tsamp.seq == 1) {
		tsamp.b_net_ipackets = tsamp.net_ipackets;
		tsamp.p_net_ipackets = tsamp.net_ipackets;

		tsamp.b_net_opackets = tsamp.net_opackets;
		tsamp.p_net_opackets = tsamp.net_opackets;

		tsamp.b_net_ibytes = tsamp.net_ibytes;
		tsamp.p_net_ibytes = tsamp.net_ibytes;

		tsamp.b_net_obytes = tsamp.net_obytes;
		tsamp.p_net_obytes = tsamp.net_obytes;
	}

	free(buf);
}

/*
 * Sample disk usage.  The algorithm used has the same limitations as that used
 * for libtop_p_networks_sample().
 */
static int
libtop_p_disks_sample(void)
{
	int retval;
	io_registry_entry_t	drive;
	io_iterator_t		drive_list;
	CFNumberRef		number;
	CFDictionaryRef		properties, statistics;
	UInt64			value;
	
	/* Get the list of all drive objects. */
	if (IOServiceGetMatchingServices(libtop_master_port,
	    IOServiceMatching("IOBlockStorageDriver"), &drive_list)) {
		libtop_print(libtop_user_data,
		    "Error in IOServiceGetMatchingServices()");
		return -1;
	}

	tsamp.p_disk_rops = tsamp.disk_rops;
	tsamp.p_disk_wops = tsamp.disk_wops;
	tsamp.p_disk_rbytes = tsamp.disk_rbytes;
	tsamp.p_disk_wbytes = tsamp.disk_wbytes;

	tsamp.disk_rops = 0;
	tsamp.disk_wops = 0;
	tsamp.disk_rbytes = 0;
	tsamp.disk_wbytes = 0;
	while ((drive = IOIteratorNext(drive_list)) != 0) {
		number = 0;
		properties = 0;
		statistics = 0;
		value = 0;

		/* Obtain the properties for this drive object. */
		if (IORegistryEntryCreateCFProperties(drive,
		    (CFMutableDictionaryRef *)&properties, kCFAllocatorDefault, 0)) {
			libtop_print(libtop_user_data,
			    "Error in IORegistryEntryCreateCFProperties()");
			retval = -1;
			goto RETURN;  // We must use a goto here to clean up drive_list 
		}

		if (properties != 0) {
			/* Obtain the statistics from the drive properties. */
			statistics
			    = (CFDictionaryRef)CFDictionaryGetValue(properties,
			    CFSTR(kIOBlockStorageDriverStatisticsKey));

			if (statistics != 0) {
				/* Get number of reads. */
				number =
				    (CFNumberRef)CFDictionaryGetValue(statistics,
				    CFSTR(kIOBlockStorageDriverStatisticsReadsKey));
				if (number != 0) {
					CFNumberGetValue(number,
					    kCFNumberSInt64Type, &value);
					tsamp.disk_rops += value;
				}

				/* Get bytes read. */
				number =
				    (CFNumberRef)CFDictionaryGetValue(statistics,
				    CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey));
				if (number != 0) {
					CFNumberGetValue(number,
					    kCFNumberSInt64Type, &value);
					tsamp.disk_rbytes += value;
				}

				/* Get number of writes. */
				number =
				    (CFNumberRef)CFDictionaryGetValue(statistics,
				    CFSTR(kIOBlockStorageDriverStatisticsWritesKey));
				if (number != 0) {
					CFNumberGetValue(number,
					    kCFNumberSInt64Type, &value);
					tsamp.disk_wops += value;
				}

				/* Get bytes written. */
				number =
				    (CFNumberRef)CFDictionaryGetValue(statistics,
				    CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey));
				if (number != 0) {
					CFNumberGetValue(number,
					    kCFNumberSInt64Type, &value);
					tsamp.disk_wbytes += value;
				}
			}

			/* Release. */
			CFRelease(properties);
		}

		/* Release. */
		IOObjectRelease(drive);
	}
	IOIteratorReset(drive_list);
	if (tsamp.seq == 1) {
		tsamp.b_disk_rops = tsamp.disk_rops;
		tsamp.p_disk_rops = tsamp.disk_rops;

		tsamp.b_disk_wops = tsamp.disk_wops;
		tsamp.p_disk_wops = tsamp.disk_wops;

		tsamp.b_disk_rbytes = tsamp.disk_rbytes;
		tsamp.p_disk_rbytes = tsamp.disk_rbytes;

		tsamp.b_disk_wbytes = tsamp.disk_wbytes;
		tsamp.p_disk_wbytes = tsamp.disk_wbytes;
	}

	retval = 0;
	RETURN:
	/* Release. */
	IOObjectRelease(drive_list);
	return retval;
}


/* Iterate through all processes and update their statistics. */
static int
libtop_p_proc_table_read(boolean_t reg)
{
	kern_return_t	kr;
	processor_set_name_array_t psets;
	processor_set_t	pset;
	task_array_t tasks;
	mach_msg_type_number_t	i, j, pcnt, tcnt;
	bool fatal_error_occurred = false;

	kr = host_processor_sets(libtop_port, &psets, &pcnt);
	if (kr != KERN_SUCCESS) {
		libtop_print(libtop_user_data, "Error in host_processor_sets(): %s",
		    mach_error_string(kr));
		return -1;
	}

	for (i = 0; i < pcnt; i++) {
		kr = host_processor_set_priv(libtop_port, psets[i], &pset);
		if (kr != KERN_SUCCESS) {
			libtop_print(libtop_user_data,
				"Error in host_processor_set_priv(): %s",
			    mach_error_string(kr));
			return -1;
		}

		kr = processor_set_tasks(pset, &tasks, &tcnt);
		if (kr != KERN_SUCCESS) {
			libtop_print(libtop_user_data, "Error in processor_set_tasks(): %s",
			    mach_error_string(kr));
			return -1;
		}
		
		tsamp.reg = 0;
		tsamp.rprvt = 0;
		tsamp.vsize = 0;
		tsamp.threads = 0;
//		libtop_p_oinfo_reset();

		for (j = 0; j < tcnt; j++) {
			switch(libtop_p_task_update(tasks[j], reg)) {
				case LIBTOP_ERR_INVALID:
					/* 
					 * The task became invalid. 
					 * The called function takes care of destroying the pinfo in the tree, if needed.
					 */
					break;

				case LIBTOP_ERR_ALLOC:
					fatal_error_occurred = true;
					break;
			}

			mach_port_deallocate(mach_task_self(), tasks[j]);
		}

		kr = mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)tasks, tcnt * sizeof(*tasks));
		kr = mach_port_deallocate(mach_task_self(), pset);
		kr = mach_port_deallocate(mach_task_self(), psets[i]);
	}

	kr = mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)psets, pcnt * sizeof(*psets));

	return (fatal_error_occurred) ? -1 : 0;
}

/*
 * Return the CPU type of the process.
 */
static libtop_status_t
libtop_p_cputype(pid_t pid, cpu_type_t *cputype) {
	int res = -1;
	static int mib[CTL_MAXNAME];
	static size_t miblen = 0;

	*cputype = 0;
	
	if (miblen == 0) {
		miblen = CTL_MAXNAME;
		res = sysctlnametomib("sysctl.proc_cputype", mib, &miblen);
		if (res != 0) {
			miblen = 0;
		}
	}

	if (miblen > 0) {
		mib[miblen] = pid;
		size_t len = sizeof(*cputype);
		res = sysctl(mib, miblen + 1, cputype, &len, NULL, 0);
	}

	/* res will be 0 if the sysctl was successful. */
	return (res == 0) ? LIBTOP_NO_ERR : LIBTOP_ERR_INVALID;
}

/*
 * Return the shared region size for an architecture.
 */
static mach_vm_size_t
libtop_p_shreg_size(cpu_type_t type) {
	switch(type)
	{
		case CPU_TYPE_ARM:			return SHARED_REGION_SIZE_ARM;
		case CPU_TYPE_POWERPC:		return SHARED_REGION_SIZE_PPC;
		case CPU_TYPE_POWERPC64:	return SHARED_REGION_SIZE_PPC64;
		case CPU_TYPE_I386:			return SHARED_REGION_SIZE_I386;
		case CPU_TYPE_X86_64:		return SHARED_REGION_SIZE_X86_64;
		default:					return 0;
	}
}

static int __attribute__((noinline))
kinfo_for_pid(struct kinfo_proc* kinfo, pid_t pid) {
	size_t miblen = 4, len;
	int mib[miblen];
	int res;
		
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = pid;
	len = sizeof(struct kinfo_proc);
	res = sysctl(mib, miblen, kinfo, &len, NULL, 0);
	if (res != 0) {
		libtop_print(libtop_user_data, "%s(): Error in sysctl(): %s",
					 __FUNCTION__, strerror(errno));
	}
	return res;
}

static kern_return_t
libtop_pinfo_update_mach_ports(task_t task, libtop_pinfo_t* pinfo) {
	kern_return_t kr;
	mach_msg_type_number_t ncnt, tcnt;
	mach_port_name_array_t names;
	mach_port_type_array_t types;

	pinfo->psamp.p_prt = pinfo->psamp.prt;

	kr = mach_port_names(task, &names, &ncnt, &types, &tcnt);
	if (kr != KERN_SUCCESS) return 0;

	pinfo->psamp.prt = ncnt;

	kr = mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)names, ncnt * sizeof(*names));
	kr = mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)types, tcnt * sizeof(*types));
	
	return kr;
}

static kern_return_t
libtop_pinfo_update_events_info(task_t task, libtop_pinfo_t* pinfo) {
	kern_return_t kr;
	mach_msg_type_number_t count = TASK_EVENTS_INFO_COUNT;

	pinfo->psamp.p_events = pinfo->psamp.events;

	pinfo->psamp.faults.previous = pinfo->psamp.faults.now;
	pinfo->psamp.pageins.previous = pinfo->psamp.pageins.now;
	pinfo->psamp.cow_faults.previous = pinfo->psamp.cow_faults.now;
	pinfo->psamp.messages_sent.previous = pinfo->psamp.messages_sent.now;
	pinfo->psamp.messages_recv.previous = pinfo->psamp.messages_recv.now;
	pinfo->psamp.syscalls_mach.previous = pinfo->psamp.syscalls_mach.now;
	pinfo->psamp.syscalls_bsd.previous = pinfo->psamp.syscalls_bsd.now;
	pinfo->psamp.csw.previous = pinfo->psamp.csw.now;
	
	kr = task_info(task, TASK_EVENTS_INFO, (task_info_t)&pinfo->psamp.events, &count);
	if (kr != KERN_SUCCESS) return kr;


#if 0
	if(pinfo->psamp.pid == 0)  {
		/* Used for testing libtop_i64_*(). */
		static int i;
		pinfo->psamp.events.faults = ((INT_MAX - pinfo->psamp.events.faults) - 20000) + i;

		/* This tests the libtop_i64_init initial overflow behavior. */
		pinfo->psamp.events.csw += INT_MAX;

		i += INT_MAX / 2;
	}
#endif

	if (pinfo->psamp.p_seq == 0) {
		pinfo->psamp.b_events = pinfo->psamp.events;
		pinfo->psamp.p_events = pinfo->psamp.events;


		pinfo->psamp.faults.i64 = libtop_i64_init(0, pinfo->psamp.events.faults);
		pinfo->psamp.pageins.i64 = libtop_i64_init(0, pinfo->psamp.events.pageins);
		pinfo->psamp.cow_faults.i64 = libtop_i64_init(0, pinfo->psamp.events.cow_faults);
		pinfo->psamp.messages_sent.i64 = libtop_i64_init(0, pinfo->psamp.events.messages_sent);
		pinfo->psamp.messages_recv.i64 = libtop_i64_init(0, pinfo->psamp.events.messages_received);
		pinfo->psamp.syscalls_mach.i64 = libtop_i64_init(0, pinfo->psamp.events.syscalls_mach);
		pinfo->psamp.syscalls_bsd.i64 = libtop_i64_init(0, pinfo->psamp.events.syscalls_unix);
		pinfo->psamp.csw.i64 = libtop_i64_init(0, pinfo->psamp.events.csw);
	}

	libtop_i64_update(&pinfo->psamp.faults.i64, pinfo->psamp.events.faults);
	libtop_i64_update(&pinfo->psamp.pageins.i64, pinfo->psamp.events.pageins);
	libtop_i64_update(&pinfo->psamp.cow_faults.i64, pinfo->psamp.events.cow_faults);
	libtop_i64_update(&pinfo->psamp.messages_sent.i64, pinfo->psamp.events.messages_sent);
	libtop_i64_update(&pinfo->psamp.messages_recv.i64, pinfo->psamp.events.messages_received);
	libtop_i64_update(&pinfo->psamp.syscalls_mach.i64, pinfo->psamp.events.syscalls_mach);
	libtop_i64_update(&pinfo->psamp.syscalls_bsd.i64, pinfo->psamp.events.syscalls_unix);
	libtop_i64_update(&pinfo->psamp.csw.i64, pinfo->psamp.events.csw);	

	pinfo->psamp.faults.now = libtop_i64_value(&pinfo->psamp.faults.i64);
	pinfo->psamp.pageins.now = libtop_i64_value(&pinfo->psamp.pageins.i64);
	pinfo->psamp.cow_faults.now = libtop_i64_value(&pinfo->psamp.cow_faults.i64);
	pinfo->psamp.messages_sent.now = libtop_i64_value(&pinfo->psamp.messages_sent.i64);
	pinfo->psamp.messages_recv.now = libtop_i64_value(&pinfo->psamp.messages_recv.i64);
	pinfo->psamp.syscalls_mach.now = libtop_i64_value(&pinfo->psamp.syscalls_mach.i64);
	pinfo->psamp.syscalls_bsd.now = libtop_i64_value(&pinfo->psamp.syscalls_bsd.i64);
	pinfo->psamp.csw.now = libtop_i64_value(&pinfo->psamp.csw.i64);
	
	return kr;
}

static kern_return_t
libtop_pinfo_update_kernmem_info(task_t task, libtop_pinfo_t* pinfo) {
	kern_return_t kr;
	
	mach_msg_type_number_t count = TASK_KERNELMEMORY_INFO_COUNT;

	pinfo->psamp.p_palloc = pinfo->psamp.palloc;
	pinfo->psamp.p_pfree = pinfo->psamp.pfree;
	pinfo->psamp.p_salloc = pinfo->psamp.salloc;
	pinfo->psamp.p_sfree = pinfo->psamp.sfree;
	
	kr = task_info(task, TASK_KERNELMEMORY_INFO, (task_info_t)&pinfo->psamp.palloc, &count);
	return kr;
}

static kern_return_t
libtop_pinfo_update_cpu_usage(task_t task, libtop_pinfo_t* pinfo, int *state) {
	kern_return_t kr;
	thread_act_array_t threads;
	mach_msg_type_number_t tcnt;

	// Update thread status
	
	*state = LIBTOP_STATE_MAX;
	pinfo->psamp.state = LIBTOP_STATE_MAX;

	kr = task_threads(task, &threads, &tcnt);
	if (kr != KERN_SUCCESS) return kr;

	if(tsamp.seq > 1) {
		pinfo->psamp.p_th = pinfo->psamp.th;
		pinfo->psamp.p_running_th = pinfo->psamp.running_th;
	} else {
		pinfo->psamp.p_th = 0;
		pinfo->psamp.p_running_th = 0;
	}
	
	pinfo->psamp.th = tcnt;
	tsamp.threads += tcnt;
	
	pinfo->psamp.running_th = 0;
	
	int i;
	for (i = 0; i < tcnt; i++) {
		thread_basic_info_data_t info;
		mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
		
		kr = thread_info(threads[i], THREAD_BASIC_INFO, (thread_info_t)&info, &count);
		if (kr != KERN_SUCCESS) continue;
		
		if ((info.flags & TH_FLAGS_IDLE) == 0) {
			struct timeval tv;
			TIME_VALUE_TO_TIMEVAL(&info.user_time, &tv);
			timeradd(&pinfo->psamp.total_time, &tv, &pinfo->psamp.total_time);
			TIME_VALUE_TO_TIMEVAL(&info.system_time, &tv);
			timeradd(&pinfo->psamp.total_time, &tv, &pinfo->psamp.total_time);
		}

		if(info.run_state == TH_STATE_RUNNING) {
			pinfo->psamp.running_th++;	
		}

		uint32_t tstate;
		tstate = libtop_p_mach_state_order(info.run_state, info.sleep_time);
		if (tstate < *state) {
			*state = tstate;
			pinfo->psamp.state = tstate;
		}

		kr = mach_port_deallocate(mach_task_self(), threads[i]);
	}
	kr = mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)(uintptr_t)threads, tcnt * sizeof(*threads));
	return kr;
}

#ifdef TOP_DEBUG_VM
static char*
share_mode_to_string(int share_mode) {
	switch(share_mode) {
		case SM_COW: return "COW";
		case SM_PRIVATE: return "PRV";
		case SM_EMPTY: return "NUL";
		case SM_SHARED: return "ALI";
		default: return "???";
	}
}
#endif

static kern_return_t
libtop_update_vm_regions(task_t task, libtop_pinfo_t* pinfo) {
	kern_return_t kr;
	
	mach_vm_size_t rprvt = 0;
	mach_vm_size_t vprvt = 0;
	mach_vm_size_t rshrd = 0;
	mach_vm_size_t empty = 0;
	mach_vm_size_t aliased = 0;
	mach_vm_size_t fw_private = 0;
	mach_vm_size_t pagesize = tsamp.pagesize;
	mach_vm_size_t regs = 0;
	
	mach_vm_address_t addr;
	mach_vm_size_t size;

	libtop_oinfo_t *oinfo;

	pinfo->split = FALSE;

#ifdef TOP_DEBUG_VM
	fprintf(stderr, "pid %d\n", pinfo->psamp.pid);
	fprintf(stderr, "range\tsize\tprivate\tshared\trc\toid\tmode\tvprvt\trprvt\trshrd\tempty\taliased\n");
#endif
	
	for (addr = 0; ; addr += size) {
		vm_region_top_info_data_t info;
		mach_msg_type_number_t count = VM_REGION_TOP_INFO_COUNT;
		mach_port_t object_name;
		
		kr = mach_vm_region(task, &addr, &size, VM_REGION_TOP_INFO, (vm_region_info_t)&info, &count, &object_name);
		if (kr != KERN_SUCCESS) break;

#ifdef TOP_DEBUG_VM
		fprintf(stderr, "%016llx-%016llx\t%lld\t%d\t%d\t%d\t%d\t%s",
				addr, addr+size, size, info.private_pages_resident, info.shared_pages_resident, info.ref_count, info.obj_id, share_mode_to_string(info.share_mode));
#endif
		
		if (in_shared_region(addr, pinfo->psamp.cputype)) {
			// Private Shared
			fw_private += info.private_pages_resident * pagesize;

			/*
			 * Check if this process has the globally shared
			 * text and data regions mapped in.  If so, set
			 * pinfo->split to TRUE and avoid checking
			 * again.
			 */
			if (pinfo->split == FALSE && info.share_mode == SM_EMPTY) {
				vm_region_basic_info_data_64_t	b_info;
				mach_vm_address_t b_addr = addr;
				mach_vm_size_t b_size = size;
				count = VM_REGION_BASIC_INFO_COUNT_64;
				
				kr = mach_vm_region(task, &b_addr, &b_size, VM_REGION_BASIC_INFO, (vm_region_info_t)&b_info, &count, &object_name);
				if (kr != KERN_SUCCESS) break;

				if (b_info.reserved) {
					pinfo->split = TRUE;
				}
			}
			
			/*
			 * Short circuit the loop if this isn't a shared
			 * private region, since that's the only region
			 * type we care about within the current address
			 * range.
			 */
			if (info.share_mode != SM_PRIVATE) {
#ifdef TOP_DEBUG_VM
				fprintf(stderr, "\n");
#endif
				continue;
			}
		}
		
		regs++;

		/*
		 * Update counters according to the region type.
		 */
		
		if (info.share_mode == SM_COW && info.ref_count == 1) {
			// Treat single reference SM_COW as SM_PRIVATE
			info.share_mode = SM_PRIVATE;
		}

		switch (info.share_mode) {
			case SM_LARGE_PAGE:
				// Treat SM_LARGE_PAGE the same as SM_PRIVATE
				// since they are not shareable and are wired.
			case SM_PRIVATE:
				rprvt += info.private_pages_resident * pagesize;
				rprvt += info.shared_pages_resident * pagesize;
				vprvt += size;
				break;
				
			case SM_EMPTY:
				empty += size;
				break;
				
			case SM_COW:
			case SM_SHARED:
				if (pinfo->psamp.pid == 0) {
					// Treat kernel_task specially
					if (info.share_mode == SM_COW) {
						rprvt += info.private_pages_resident * pagesize;
						vprvt += size;
					}
					break;
				}
					
				oinfo = libtop_p_oinfo_insert(info.obj_id, info.share_mode, info.shared_pages_resident, info.ref_count, size, pinfo);
				if (!oinfo) {
					return -1;
				}

				// roll back if necessary
				if (oinfo->proc_ref_count > 1) {
					if (oinfo->rb_share_type != SM_EMPTY) {
						oinfo->share_type = oinfo->rb_share_type;
					}
					aliased -= oinfo->rb_aliased;
					vprvt -= oinfo->rb_vprvt;
					rshrd -= oinfo->rb_rshrd;
				}
				// clear rollback fields
				oinfo->rb_share_type = SM_EMPTY;
				oinfo->rb_aliased = 0;
				oinfo->rb_vprvt = 0;
				oinfo->rb_rshrd = 0;
				
				// XXX: SM_SHARED make sense here for the SM_COW case?
				if (oinfo->share_type == SM_SHARED && oinfo->ref_count == oinfo->proc_ref_count) {
					// Private aliased object
					oinfo->rb_share_type = oinfo->share_type;
					oinfo->share_type = SM_PRIVATE_ALIASED;
					
					oinfo->rb_aliased += oinfo->resident_page_count * pagesize;
					aliased += oinfo->resident_page_count * pagesize;
					
					oinfo->rb_vprvt += oinfo->size;
					vprvt += oinfo->size;
				}
				
				if (oinfo->share_type != SM_PRIVATE_ALIASED) {
					oinfo->rb_rshrd += oinfo->resident_page_count * pagesize;
					rshrd += oinfo->resident_page_count * pagesize;
				}
				
				if (info.share_mode == SM_COW) {
					rprvt += info.private_pages_resident * pagesize;
					vprvt += info.private_pages_resident * pagesize; // XXX: size?
				}
				break;

			default:
				assert(0);
				break;
		}
#ifdef TOP_DEBUG_VM
		fprintf(stderr, "\t%lld\t%lld\t%lld\t%lld\t%lld\n", vprvt, rprvt, rshrd, empty, aliased);
#endif
	}

	pinfo->psamp.rprvt = rprvt;
	pinfo->psamp.vprvt = vprvt;
	pinfo->psamp.rshrd = rshrd;
	pinfo->psamp.empty = empty;
	pinfo->psamp.rprvt += aliased;
	pinfo->psamp.fw_private = fw_private;
	
	if(tsamp.seq > 1) {
		pinfo->psamp.p_reg = pinfo->psamp.reg;
	} else {
		pinfo->psamp.p_reg = 0;
	}
	pinfo->psamp.reg = regs;

	return kr;
}

/*
 * This may destroy the pinfo associated with a pid/task.
 * The caller should not make assumptions about the lifetime of the pinfo.
 */
static libtop_status_t __attribute__((noinline))
libtop_p_task_update(task_t task, boolean_t reg)
{
	kern_return_t kr;
	int res;
	struct kinfo_proc	kinfo;
	pid_t pid;
	mach_msg_type_number_t	count;
	libtop_pinfo_t		*pinfo;
	struct task_basic_info_64	ti;
	int			state;

	state = LIBTOP_STATE_ZOMBIE;

	/* Get pid for this task. */
	kr = pid_for_task(task, &pid);
	if (kr != KERN_SUCCESS) {
		return LIBTOP_ERR_INVALID;
	}

	res = kinfo_for_pid(&kinfo, pid);
	if (res != 0) {
		return LIBTOP_ERR_INVALID;	
	}

	if (kinfo.kp_proc.p_stat == SZOMB) {
		/* Zombie process. */
	}

	/*
	 * Search for the process.  If we haven't seen it before, allocate and
	 * insert a new pinfo structure.
	 */
	pinfo = libtop_p_psearch((pid_t)pid);
	if (pinfo == NULL) {
		pinfo = (libtop_pinfo_t *)calloc(1, sizeof(libtop_pinfo_t));
		if (pinfo == NULL) {
			return LIBTOP_ERR_ALLOC;
		}
		pinfo->psamp.pid = (pid_t)pid;
		libtop_p_pinsert(pinfo);
	}

	/* 
	 * Get command name/args. 
	 * This is expected to return a LIBTOP_ERR* or LIBTOP_NO_ERR (when successful).
	 */
	res = libtop_p_proc_command(pinfo, &kinfo);
	switch(res) {
		case LIBTOP_NO_ERR: 
			break;

		case LIBTOP_ERR_INVALID:
			/*
			 * Something failed while fetching the command string (the pid is probably gone).
			 * Remove the pinfo struct from the tree, and free it with destroy_pinfo.
			 */
			libtop_p_destroy_pinfo(pinfo);
			return res;
	
		case LIBTOP_ERR_ALLOC:
			return res;
	}

	pinfo->psamp.uid = kinfo.kp_eproc.e_ucred.cr_uid;
	pinfo->psamp.ppid = kinfo.kp_eproc.e_ppid;
	pinfo->psamp.pgrp = kinfo.kp_eproc.e_pgid;
	pinfo->flag = kinfo.kp_proc.p_flag;
	pinfo->psamp.started = kinfo.kp_proc.p_starttime;

	pinfo->psamp.p_seq = pinfo->psamp.seq;
	pinfo->psamp.seq = tsamp.seq;

	/*
	 * 6255752: CPU type of a process can change due to a re-exec.
	 * We may see some transient oddities due to this behavior, but
	 * it should stabilize after one refresh.
	 */
	if(LIBTOP_NO_ERR != libtop_p_cputype(pinfo->psamp.pid, &pinfo->psamp.cputype)) {
		/* 
		 * This pid is most likely invalid now, because the 
		 * cputype couldn't be found.
		 *
		 */
		
		libtop_p_destroy_pinfo(pinfo);		
		return LIBTOP_ERR_INVALID;
	}

	/*
	 * Get task_info, which is used for memory usage and CPU usage
	 * statistics.
	 */
	count = TASK_BASIC_INFO_64_COUNT;
	kr = task_info(task, TASK_BASIC_INFO_64, (task_info_t)&ti, &count);
	if (kr != KERN_SUCCESS) {
		libtop_p_destroy_pinfo(pinfo);
		return LIBTOP_ERR_INVALID;
	}

	/*
	 * Get memory usage statistics.
	 */

	/* Make copies of previous sample values. */
	pinfo->psamp.p_rsize = pinfo->psamp.rsize;
	pinfo->psamp.p_vsize = pinfo->psamp.vsize;
	pinfo->psamp.p_rprvt = pinfo->psamp.rprvt;
	pinfo->psamp.p_vprvt = pinfo->psamp.vprvt;
	pinfo->psamp.p_rshrd = pinfo->psamp.rshrd;
	pinfo->psamp.p_empty = pinfo->psamp.empty;

	/* Clear sizes in preparation for determining their current values. */
	//pinfo->psamp.rprvt = 0;
	//pinfo->psamp.vprvt = 0;
	//pinfo->psamp.rshrd = 0;
	//pinfo->psamp.empty = 0;
	//pinfo->psamp.reg = 0;
	//pinfo->psamp.fw_private = 0;
	
	/*
	 * Do memory object traversal if any of the following is true:
	 *
	 * 1) Region reporting is enabled for this sample, and it isn't
	 *    explicitly disabled for this process.
	 *
	 * 2) Region reporting is explicitly enabled for this process.
	 *
	 * 3) This is the first sample for this process.
	 *
	 * 4) A previous sample detected that the globally shared text and data
	 *    segments were mapped in, but if we were to subtract them out,
	 *    the process's calculated vsize would be less than 0.
	 */
	
	if ((reg && pinfo->preg != LIBTOP_PREG_off)
		|| pinfo->preg == LIBTOP_PREG_on
		|| pinfo->psamp.p_seq == 0
		|| (pinfo->split && ti.virtual_size
		    < libtop_p_shreg_size(pinfo->psamp.cputype))) {
	
		libtop_update_vm_regions(task, pinfo);
	} 

	/* 
	 * These need to be copied regardless of the region reporting above. 
	 * The previous (p_) values were copied earlier.
	 */
	pinfo->psamp.rsize = ti.resident_size;
	pinfo->psamp.vsize = ti.virtual_size;

	// Update total time.
	pinfo->psamp.p_total_time = pinfo->psamp.total_time;

	struct timeval tv;
	TIME_VALUE_TO_TIMEVAL(&ti.user_time, &pinfo->psamp.total_time);
	TIME_VALUE_TO_TIMEVAL(&ti.system_time, &tv);
	timeradd(&pinfo->psamp.total_time, &tv, &pinfo->psamp.total_time);
	
	/*
	 * Get CPU usage statistics.
	 */
	kr = libtop_pinfo_update_cpu_usage(task, pinfo, &state);

	if (pinfo->psamp.p_seq == 0) {
		/* Set initial values. */
		pinfo->psamp.b_total_time = pinfo->psamp.total_time;
		pinfo->psamp.p_total_time = pinfo->psamp.total_time;
	}
	
	/*
	 * Get number of Mach ports.
	 */
	kr = libtop_pinfo_update_mach_ports(task, pinfo);

	/*
	 * Get event counters.
	 */
	kr = libtop_pinfo_update_events_info(task, pinfo);

	/*
	 * Get updated wired memory usage numbers
	 */
	kr = libtop_pinfo_update_kernmem_info(task, pinfo);

	libtop_p_wq_update(pinfo);
	
	tsamp.state_breakdown[state]++;

	return LIBTOP_NO_ERR;
}

/*
 * Get the command name for the process associated with pinfo.  For CFM
 * applications, this requires substantial extra work, since the basename of the
 * first program argument is the actual command name.
 *
 * Due to limitations in the KERN_PROCARGS sysctl as implemented in OS X 10.2,
 * changes were made to the sysctl to make finding the process arguments more
 * deterministic.  If TOP_JAGUAR is defined, the old algorithm is used, rather
 * than the simpler new one.
 */
static libtop_status_t
libtop_p_proc_command(libtop_pinfo_t *pinfo, struct kinfo_proc *kinfo)
{
#if 0
	uint32_t bufferSize = 30;
	int result;

	if (pinfo->psamp.command) {
		free(pinfo->psamp.command);
		pinfo->psamp.command = NULL;
	}

	pinfo->psamp.command = malloc(bufferSize);
	if (NULL == pinfo->psamp.command)
		return LIBTOP_ERR_ALLOC;

	pinfo->psamp.command[0] = '\0';
	proc_name(pinfo->psamp.pid, pinfo->psamp.command, bufferSize); 	
#if 0
	if(result) {
		pinfo->psamp.command[0] = '\0';
		return LIBTOP_ERR_INVALID;
	}
#endif
	
	return 0;
#endif


	uint32_t	len;

	if (pinfo->psamp.command) {
		free(pinfo->psamp.command);
		pinfo->psamp.command = NULL;
	}

	len = strlen(kinfo->kp_proc.p_comm);

	if (strncmp(kinfo->kp_proc.p_comm, "LaunchCFMApp",len) != 0) {
		/* Normal program. */
		pinfo->psamp.command = strdup(kinfo->kp_proc.p_comm);
		if (!pinfo->psamp.command) {
			return LIBTOP_ERR_ALLOC;
		}
	} else {
		int	mib[3];
		size_t	procargssize;
#ifdef TOP_JAGUAR
		char 	*arg_end, *exec_path;
		int	*ip;
#else
		char	*cp;
#endif
		char	*command_beg, *command, *command_end;

		assert(pinfo->psamp.pid != 0);

		/*
		 * CFM application.  Get the basename of the first argument and
		 * use that as the command string.
		 */

		/*
		 * Make a sysctl() call to get the raw argument space of the
		 * process.  The layout is documented in start.s, which is part
		 * of the Csu project.  In summary, it looks like:
		 *
		 * /---------------\ 0x00000000
		 * :               :
		 * :               :
		 * |---------------|
		 * | argc          |
		 * |---------------|
		 * | arg[0]        |
		 * |---------------|
		 * :               :
		 * :               :
		 * |---------------|
		 * | arg[argc - 1] |
		 * |---------------|
		 * | 0             |
		 * |---------------|
		 * | env[0]        |
		 * |---------------|
		 * :               :
		 * :               :
		 * |---------------|
		 * | env[n]        |
		 * |---------------|
		 * | 0             |
		 * |---------------| <-- Beginning of data returned by sysctl()
		 * | exec_path     |     is here.
		 * |:::::::::::::::|
		 * |               |
		 * | String area.  |
		 * |               |
		 * |---------------| <-- Top of stack.
		 * :               :
		 * :               :
		 * \---------------/ 0xffffffff
		 */
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROCARGS2;
		mib[2] = pinfo->psamp.pid;

		procargssize = libtop_argmax;
#ifdef TOP_JAGUAR
		/* Hack to avoid kernel bug. */
		if (procargssize > 8192) {
			procargssize = 8192;
		}
#endif
		if (sysctl(mib, 3, libtop_arg, &procargssize, NULL, 0) == -1) {
			libtop_print(libtop_user_data, "%s(): Error in sysctl(): %s",
			    __FUNCTION__, strerror(errno));
			/* sysctl probably failed due to an invalid pid. */
			return LIBTOP_ERR_INVALID;
		}

#ifdef TOP_JAGUAR
		/* Set ip just above the end of libtop_arg. */
		arg_end = &libtop_arg[procargssize];
		ip = (int *)arg_end;

		/*
		 * Skip the last 2 words, since the last is a 0 word, and
		 * the second to last may be as well, if there are no
		 * arguments.
		 */
		ip -= 3;

		/* Iterate down the arguments until a 0 word is found. */
		for (; *ip != 0; ip--) {
			if (ip == (int *)libtop_arg) {
				goto ERROR;
			}
		}

		/* The saved exec_path is just above the 0 word. */
		ip++;
		exec_path = (char *)ip;

		/*
		 * Get the beginning of the first argument.  It is word-aligned,
		 * so skip padding '\0' bytes.
		 */
		command_beg = exec_path + strlen(exec_path);
		for (; *command_beg == '\0'; command_beg++) {
			if (command_beg >= arg_end) {
				goto ERROR;
			}
		}

		/* Get the basename of command. */
		command = command_end = command_beg + strlen(command_beg) + 1;
		for (command--; command >= command_beg; command--) {
			if (*command == '/') {
				break;
			}
		}
		command++;

		/* Allocate space for the command and copy. */
		len = command_end - command;
		pinfo->psamp.command = (char *)malloc(len + 1);
		if (pinfo->psamp.command == NULL) {
			return LIBTOP_ERR_ALLOC;
		}
		memcpy(pinfo->psamp.command, command, len + 1);
#else
		/* Skip the saved exec_path. */
		for (cp = libtop_arg; cp < &libtop_arg[procargssize]; cp++) {
			if (*cp == '\0') {
				/* End of exec_path reached. */
				break;
			}
		}
		if (cp == &libtop_arg[procargssize]) {
			goto ERROR;
		}

		/* Skip trailing '\0' characters. */
		for (; cp < &libtop_arg[procargssize]; cp++) {
			if (*cp != '\0') {
				/* Beginning of first argument reached. */
				break;
			}
		}
		if (cp == &libtop_arg[procargssize]) {
			goto ERROR;
		}
		command_beg = cp;

		/*
		 * Make sure that the command is '\0'-terminated.  This protects
		 * against malicious programs; under normal operation this never
		 * ends up being a problem..
		 */
		for (; cp < &libtop_arg[procargssize]; cp++) {
			if (*cp == '\0') {
				/* End of first argument reached. */
				break;
			}
		}
		if (cp == &libtop_arg[procargssize]) {
			goto ERROR;
		}
		command_end = command = cp;

		/* Get the basename of command. */
		for (command--; command >= command_beg; command--) {
			if (*command == '/') {
				command++;
				break;
			}
		}

		/* Allocate space for the command and copy. */
		len = command_end - command;
		pinfo->psamp.command = (char *)malloc(len + 1);
		if (pinfo->psamp.command == NULL) {
			return LIBTOP_ERR_ALLOC;
		}
		memcpy(pinfo->psamp.command, command, len + 1);
#endif
	}

	return LIBTOP_NO_ERR;

ERROR:
	/*
	 * Unable to parse the arguments.  Set the command name to
	 * "(LaunchCFMApp)".
	 */
	pinfo->psamp.command = strdup("(LaunchCFMApp)");
	return LIBTOP_NO_ERR;
}

/* Insert a pinfo structure into the pid-ordered tree. */
static void
libtop_p_pinsert(libtop_pinfo_t *pinfo)
{
	rb_node_new(&libtop_ptree, pinfo, pnode);
	rb_insert(&libtop_ptree, pinfo, libtop_p_pinfo_pid_comp,
	    libtop_pinfo_t, pnode);
}

/* Remove a pinfo structure from the pid-ordered tree. */
static void
libtop_p_premove(libtop_pinfo_t *pinfo)
{
	rb_remove(&libtop_ptree, pinfo, libtop_pinfo_t, pnode);
}

/* Search for a pinfo structure with pid pid. */
static libtop_pinfo_t *
libtop_p_psearch(pid_t pid)
{
	libtop_pinfo_t	*retval, key;

	key.psamp.pid = pid;
	rb_search(&libtop_ptree, &key, libtop_p_pinfo_pid_comp, pnode, retval);
	if (retval == rb_tree_nil(&libtop_ptree)) {
		retval = NULL;
	}

	return retval;
}

/*
 * Compare two pinfo structures according to pid.  This function is used for
 * operations on the pid-sorted tree of pinfo structures.
 */
static int
libtop_p_pinfo_pid_comp(libtop_pinfo_t *a, libtop_pinfo_t *b)
{
	assert(a != NULL);
	assert(b != NULL);
	if (a->psamp.pid < b->psamp.pid) return -1;
	if (a->psamp.pid > b->psamp.pid) return 1;
	return 0;
}

/* Process comparison wrapper function, used by the red-black tree code. */
static int
libtop_p_pinfo_comp(libtop_pinfo_t *a, libtop_pinfo_t *b)
{
	return libtop_sort(libtop_sort_data, &a->psamp, &b->psamp);
}

/* Insert or update a memory object info entry. */
static libtop_oinfo_t *
libtop_p_oinfo_insert(int obj_id, int share_type, int resident_page_count,
    int ref_count, int size, libtop_pinfo_t *pinfo)
{
	const void *k = (const void *)(intptr_t)obj_id;
	libtop_oinfo_t *oinfo = (libtop_oinfo_t *)CFDictionaryGetValue(libtop_oinfo_hash, k);
	int clear_rollback = 0;

	if (oinfo != NULL) {
		/* Use existing record. */
		if (oinfo->pinfo != pinfo) {
			oinfo->proc_ref_count = 0;
			oinfo->pinfo = pinfo;
			clear_rollback = 1;
		}

		oinfo->size += size;
		oinfo->proc_ref_count++;
	} else {
		oinfo = (libtop_oinfo_t *)malloc(sizeof(libtop_oinfo_t));

		if (oinfo == NULL)
			return NULL;

		oinfo->pinfo = pinfo;
		oinfo->obj_id = obj_id;
		oinfo->size = size;
		oinfo->share_type = share_type;
		oinfo->resident_page_count = resident_page_count;
		oinfo->ref_count = ref_count;
		oinfo->proc_ref_count = 1;
		clear_rollback = 1;
		
		CFDictionarySetValue(libtop_oinfo_hash, k, oinfo);
	}
	
	if (clear_rollback) {
		// clear rollback fields
		oinfo->rb_share_type = SM_EMPTY;
		oinfo->rb_aliased = 0;
		oinfo->rb_vprvt = 0;
		oinfo->rb_rshrd = 0;
	}

	return oinfo;
}

static void
libtop_p_destroy_pinfo(libtop_pinfo_t *pinfo) {
	libtop_p_premove(pinfo);

	if(pinfo->psamp.command) {
		free(pinfo->psamp.command);
		/* Guard against reuse even though we are freeing. */
		pinfo->psamp.command = NULL;
	}

	free(pinfo);
}

static int
libtop_p_wq_update(libtop_pinfo_t *pinfo) {
#ifdef PROC_PIDWORKQUEUEINFO
	struct proc_workqueueinfo wqinfo;
	int result;
#endif

	if (tsamp.seq > 1) {
		pinfo->psamp.p_wq_nthreads = pinfo->psamp.wq_nthreads;
		pinfo->psamp.p_wq_run_threads = pinfo->psamp.wq_run_threads;
		pinfo->psamp.p_wq_blocked_threads = pinfo->psamp.wq_blocked_threads;
	}

	pinfo->psamp.wq_nthreads = 0;
	pinfo->psamp.wq_run_threads = 0;
	pinfo->psamp.wq_blocked_threads = 0;

#ifdef PROC_PIDWORKQUEUEINFO
	result = proc_pidinfo(pinfo->psamp.pid, PROC_PIDWORKQUEUEINFO,
			/*arg is UNUSED with this flavor*/ 0ULL,
			&wqinfo, PROC_PIDWORKQUEUEINFO_SIZE);
	if(!result)
		return -1; /*error*/
    	
	pinfo->psamp.wq_nthreads = wqinfo.pwq_nthreads;
	pinfo->psamp.wq_run_threads = wqinfo.pwq_runthreads;
	pinfo->psamp.wq_blocked_threads = wqinfo.pwq_blockedthreads;
#endif
	
	return 0;
}

libtop_i64_t
libtop_i64_init(uint64_t acc, int last_value) {
	libtop_i64_t r;

	r.accumulator = acc;
	r.last_value = last_value;

	if(0 == acc) {
		if(last_value >= 0) {
			r.accumulator += last_value;
		} else {
			/* The initial value has already overflowed. */
			r.accumulator += INT_MAX;
			r.accumulator += abs(INT_MIN - last_value) + 1;
		}
	}

	return r;
}

void
libtop_i64_update(struct libtop_i64 *i, int value) {
	int delta = 0;

	if(value >= 0) {
		if(i->last_value >= 0) {
			delta = value - i->last_value;
		} else {
			delta = value + (-i->last_value);
		}
	} else {
		if(i->last_value >= 0) {
			delta = abs(INT_MIN - value) + 1;
			delta += INT_MAX - i->last_value;
		} else {
			delta = value - i->last_value;
		}
	} 
	
	i->accumulator += delta;

#ifdef LIBTOP_I64_DEBUG
	if(delta == 0)
		assert((value - i->last_value) == 0);

	fprintf(stderr, "%s: delta %u  :: value %d  :: i->last_value %d\n", __func__,
		delta, value, i->last_value);
	fprintf(stderr, "%s: accumulator > INT_MAX %s\n", __func__,
		(i->accumulator > INT_MAX) ? "YES" : "NO");
	
	fprintf(stderr, "%s: value %x value unsigned %u > UINT_MAX %s\n", __func__,
		value, value, (i->accumulator > UINT_MAX) ? "YES" : "NO");
#endif

	i->last_value = value;
}

uint64_t 
libtop_i64_value(libtop_i64_t *i) {
	return i->accumulator;
}

/* This tests every branch in libtop_i64_update(). */
static void
libtop_i64_test(void) {
	libtop_i64_t i64;
	int i;
	int value = 3;
	int incr = INT_MAX / 4;
	uint64_t accum = value;

	i64 = libtop_i64_init(0, value);
	
	i = 0;

	while(1) {
		fprintf(stderr, "test: i %d value %d i64 value %" PRIu64 " matches? %" PRIu64 "\n", i, value, libtop_i64_value(&i64), accum);
		fprintf(stderr, "+ %d\n", incr);
		value += incr;
		accum += incr;
		libtop_i64_update(&i64, value);
		++i;
	}
	
}
