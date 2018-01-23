#include <string.h>
#include <stdio.h>
#include <x86.h>
#include <kernel/console.h>
#include <kernel/monitor.h>

#define NESTED_TIME_TEST 5

void
init(void)
{
	extern char edata[], end[];

	/*
	* Before doing anything else, complete the ELF loading process.
	* Clear the uninitialized global data (BSS) section of our program.
	* This ensures that all static/global variables start out zero.
	*/
	memset(edata, 0, end - edata);

	/*
	* Initialize the console.
	* Can't call printf until after we do this!
	*/
	cons_init();

	cprintf("Enter toynix...\n");

	// Drop into the kernel monitor.
	while (1)
		monitor(NULL);
}

