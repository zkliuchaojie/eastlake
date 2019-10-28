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

TEST(po_close, simple_test)
{
	char poname[] = "l";
	int pod;

	pod = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod, 0);

	pod = syscall(403, pod);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname, 0);
	ASSERT_EQ(pod, 0);
}

TEST(po_close, bad_pod_test)
{
	int retval;

	retval = syscall(403, -1);
	ASSERT_LT(retval, 0);
	ASSERT_EQ(errno, EBADF);
}
