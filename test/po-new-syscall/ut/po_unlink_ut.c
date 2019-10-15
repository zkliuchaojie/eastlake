/*
 * Author: liuchaojie
 */


#include "gtest/gtest.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

TEST(po_unlink, simple_test)
{
	char poname[] = "eastlake";
	int pod;

	pod = syscall(401, poname);

	ASSERT_GE(pod, 0);
}

TEST(po_unlink, invalid_argument_test)
{
	char poname[] = "eastlake/";
	int pod;

	pod = syscall(401, poname);

	ASSERT_EQ(pod, -1);
	ASSERT_EQ(errno, EINVAL);
}
