/*
 * Author: liuchaojie
 */

#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include "ctest/ctest.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */

TEST(po_unlink, simple_test)
{
	char poname[] = "u";
	int pod;

	pod = syscall(400, poname, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(403, pod, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname);
	ASSERT_EQ(pod, 0);
}

TEST(po_unlink, busy_test)
{
	char poname[] = "u";
	int pod;
	int retval;

	pod = syscall(400, poname, 0);
	ASSERT_GE(pod, 0);

	retval = syscall(401, poname, 0);
	ASSERT_EQ(retval, -1);
	ASSERT_EQ(errno, EBUSY);

	pod = syscall(403, pod, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname);
	ASSERT_EQ(pod, 0);
}


TEST(po_unlink, invalid_argument_test)
{
	char poname[] = "u/";
	int pod;

	pod = syscall(401, poname);

	ASSERT_EQ(pod, -1);
	ASSERT_EQ(errno, EINVAL);
}
