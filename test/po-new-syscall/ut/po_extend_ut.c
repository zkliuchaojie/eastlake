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

TEST(po_extend, simple_test)
{
	char poname[] = "e";
	int pod1;
	long long retval1;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'a';
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, extend_twice_test)
{
	char poname[] = "e";
	int pod1;
	long long retval1;
	char *c1, *c2;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);
	c1[0] = 'a';
	//printf("%c\n", c[0]);
	
	c2 = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c2, 0);
	c2[0] = 'a';
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, exceed_MAX_BUDDY_ALLOC_SIZE)
{
	char poname[] = "e";
	int pod1;
	long long retval1;
	char *c1, *c2;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 2*MAX_BUDDY_ALLOC_SIZE, \
		PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);
	c1[0] = 'a';
	//printf("%c\n", c[0]);
/*
	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
*/
}
