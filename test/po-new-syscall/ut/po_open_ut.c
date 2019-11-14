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
	char poname[] = "o";
	int pod1, pod2;
	int retval1, retval2;

	pod1 = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod1, 0);

	pod2 = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod2, 1);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval2 = syscall(403, pod2, 0);
	ASSERT_GE(retval2, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}
