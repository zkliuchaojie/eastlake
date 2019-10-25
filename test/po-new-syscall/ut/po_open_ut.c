/*
 * Author: liuchaojie
 */


#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include "ctest/ctest.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

TEST(po_open, simple_test)
{
	char poname[] = "eastlake";
	int pod;

	pod = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod, 0);

	pod = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod, 1);
}
