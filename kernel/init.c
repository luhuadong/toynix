#include <string.h>
#include <stdio.h>
#include <x86.h>
#include <kernel/console.h>
#include <kernel/monitor.h>
#include <kernel/pmap.h>
#include <kernel/env.h>
#include <kernel/trap.h>
#include <kernel/cpu.h>
#include <kernel/picirq.h>
#include <kernel/spinlock.h>
#include <kernel/sched.h>
#include <kernel/pci.h>
#include <kernel/time.h>

static void boot_aps(void);

void
init(void)
{
	/*
	* Initialize the console.
	* Can't call printf until after we do this!
	*/
	cons_init();
	cprintf("Enter toynix...\n");

	mem_init();
	env_init();
	trap_init();

	/* multiprocessor initialization functions */
	/* config lapicaddr */
	mp_init();
	/* map lapic */
	lapic_init();

	/* multitasking initialization functions */
	pic_init();

	/* hardware initializations */
	time_init();
	pci_init();

	// Acquire the big kernel lock before waking up APs
	lock_kernel();
	// Starting non-boot CPUs
	boot_aps();

	// Start fs env
	ENV_CREATE(fs_fs, ENV_TYPE_FS);

#if !defined(TEST_NO_NS)
	// Start ns.
	ENV_CREATE(net_ns, ENV_TYPE_NS);
#endif

#if defined(TEST)
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	ENV_CREATE(user_initsh, ENV_TYPE_USER);
#endif

	// Schedule and run the first user environment
	sched_yield();
}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

// Start the non-boot (AP) processors.
static void
boot_aps(void)
{
	extern unsigned char mpentry_start[], mpentry_end[];
	void *code;
	struct CpuInfo *c;

	// Write entry code to unused memory at MPENTRY_PADDR
	code = KADDR(MPENTRY_PADDR);
	memmove(code, mpentry_start, mpentry_end - mpentry_start);

	// Boot each AP one at a time
	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == cpus + cpunum())  // We've started already.
			continue;

		// Tell mpentry.S what stack to use.
		// We can't use KSTACKTOP because AP can't use kern_pgdir yet (only entry_pgdir).
		mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;

		// Start the CPU at mpentry_start
		lapic_startap(c->cpu_id, PADDR(code));

		// Wait for the CPU to finish some basic setup in mp_main()
		while (c->cpu_status != CPU_STARTED);
	}
}

// Setup code for APs
void
mp_main(void)
{
	// We are in high EIP now, safe to switch to kern_pgdir 
	lcr3(PADDR(kern_pgdir));
	cprintf("SMP: CPU %d starting\n", cpunum());

	lapic_init();
	env_init_percpu();
	trap_init_percpu();
	xchg(&thiscpu->cpu_status, CPU_STARTED); // tell boot_aps() we're up

	// Now that we have finished some basic setup, call sched_yield()
	// to start running processes on this CPU.  But make sure that
	// only one CPU can enter the scheduler at a time!
	lock_kernel();
	sched_yield();
}
