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
#include <sys/mman.h>

#define PAGE_SIZE_REDEFINED	(4*1024)
#define MAX_BUDDY_ALLOC_SIZE    (PAGE_SIZE_REDEFINED << 10)

TEST(po_chunk_next, simple_test)
{
	char poname[] = "a";
	int pod1;
	long long retval1;
	char *c;
	unsigned long addrbuf[1];

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);

	retval1 = syscall(410, pod1, NULL, 1, addrbuf);
	ASSERT_EQ(retval1, 0);
	ASSERT_EQ(c, addrbuf[0]);

	retval1 = syscall(410, pod1, c, 1, addrbuf);
	ASSERT_EQ(retval1, 0);
	ASSERT_EQ(NULL, addrbuf[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_chunk_next, extend_twice_test)
{
	char poname[] = "b";
	int pod1;
	long long retval1;
	char *c1, *c2;
	unsigned long addrbuf[2];

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);

	c2 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c2, 0);

	retval1 = syscall(410, pod1, NULL, 1, addrbuf);
	ASSERT_EQ(retval1, 0);
	ASSERT_EQ(c1, addrbuf[0]);

	retval1 = syscall(410, pod1, NULL, 2, addrbuf);
	ASSERT_EQ(retval1, 0);
	ASSERT_EQ(c1, addrbuf[0]);
	ASSERT_EQ(c2, addrbuf[1]);

	retval1 = syscall(410, pod1, c1, 2, addrbuf);
	ASSERT_EQ(retval1, 0);
	ASSERT_EQ(c2, addrbuf[0]);
	ASSERT_EQ(NULL, addrbuf[1]);

	retval1 = syscall(410, pod1, c2, 2, addrbuf);
	ASSERT_EQ(retval1, 0);
	ASSERT_EQ(NULL, addrbuf[0]);
	ASSERT_EQ(NULL, addrbuf[1]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}
