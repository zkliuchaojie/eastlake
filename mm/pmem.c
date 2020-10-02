/*
 * Author: liuchaojie
 */

#include <linux/pmem.h>
#include <linux/syscalls.h>

/* this syscall is used to debug persistent memory management */
SYSCALL_DEFINE1(debugger, unsigned int, op)
{
	if (op == 0)
		return 0;
	return op;
}

