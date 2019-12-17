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

TEST(po_shrink, simple_test)
{
	char poname[] = "x";
	int pod1;
	long long retval1;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);
	
	retval1 = syscall(407, pod1, c, 4096);
	ASSERT_EQ(retval1, 0);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

/*
 * TODO: make it automatic.
 */
TEST(po_shrink, can_not_access_after_shrink_test)
{
	char poname[] = "x";
	int pod1;
	long long retval1;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'x';

	retval1 = syscall(407, pod1, c, 4096);
	ASSERT_EQ(retval1, 0);
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_shrink, extend_twice_and_shrink_once_test)
{
	char poname[] = "x";
	int pod1;
	long long retval1;
	char *c1, *c2;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);

	c2 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c2, 0);

	retval1 = syscall(407, pod1, c1, 4096);
	ASSERT_EQ(retval1, 0);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_shrink, extend_twice_and_shrink_once_but_the_last_one_test)
{
	char poname[] = "x";
	int pod1;
	long long retval1;
	char *c1, *c2;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);

	c2 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c2, 0);

	retval1 = syscall(407, pod1, c2, 4096);
	ASSERT_EQ(retval1, 0);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_shrink, exceed_MAX_BUDDY_ALLOC_SIZE)
{
	char poname[] = "x";
	int pod1;
	long long retval1;
	char *c1;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 2*MAX_BUDDY_ALLOC_SIZE, \
		PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);
	c1[0] = 'a';
	//printf("%c\n", c[0]);

	retval1 = syscall(407, pod1, c1, 2*MAX_BUDDY_ALLOC_SIZE);
	ASSERT_EQ(retval1, 0);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);

}

