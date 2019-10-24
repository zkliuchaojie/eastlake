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
	char poname[] = "eastlake";
	int pod;

	pod = syscall(400, poname, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname);
	ASSERT_EQ(pod, 0);
}

TEST(po_unlink, invalid_argument_test)
{
	char poname[] = "eastlake/";
	int pod;

	pod = syscall(401, poname);

	ASSERT_EQ(pod, -1);
	ASSERT_EQ(errno, EINVAL);
}
