/*
 * Author: liuchaojie
 */


#include "gtest/gtest.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

TEST(po_creat, simple_test)
{
	char poname[] = "eastlake";
	int pod;

	pod = syscall(400, poname, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname, 0);
	ASSERT_EQ(pod, 0);
}

TEST(po_creat, invalid_argument_test)
{
	char poname[] = "eastlake/";
	int pod;

	pod = syscall(400, poname, 0);

	ASSERT_EQ(pod, -1);
	ASSERT_EQ(errno, EINVAL);
}

TEST(po_creat, poname_too_long_test)
{
	char poname[256];
	int i, pod;

	for (i = 0; i < 255; i++)
		poname[i] = 'e';

	poname[255] = '\0';
	pod = syscall(400, poname, 0);
	ASSERT_GE(pod, 0);

	poname[255] = 'e';
	pod = syscall(400, poname, 0);
	ASSERT_EQ(pod, -1);
	ASSERT_EQ(errno, ENAMETOOLONG);
}
