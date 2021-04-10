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

#define MAP_NUMA_AWARE         0x400000

#define PAGE_SIZE_REDEFINED	(4*1024UL)
#define MAX_BUDDY_ALLOC_SIZE    (PAGE_SIZE_REDEFINED << 15)

TEST(po_extend, simple_test)
{
	char poname[] = "f";
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
	char poname[] = "f";
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
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c1, *c2;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 10*MAX_BUDDY_ALLOC_SIZE, \
		PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);
	c1[0] = 'a';
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);

}

TEST(po_extend, not_aligned_with_MAX_BUDDY_ALLOC_SIZE)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c1, *c2;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c1 = (char *)syscall(406, pod1, 2*MAX_BUDDY_ALLOC_SIZE + PAGE_SIZE_REDEFINED, \
		PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c1, 0);
	c1[0] = 'a';
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);

}

TEST(po_extend, extend_twice_both_bigger_than_MAX_BUDDY_ALLOC_SIZE)
{
	char poname[] = "f";
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

	c2 = (char *)syscall(406, pod1, 2*MAX_BUDDY_ALLOC_SIZE,  \
		PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c2, 0);
	c2[0] = 'b';
	//printf("%c\n", c[0]);
	ASSERT_EQ(c1[0], 'a');

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, extend_addr_should_not_be_the_same)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c_array[10];
	int i, j;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);
	for (i = 0; i < 10; i++) {
		c_array[i] = (char *)syscall(406, pod1, 2*MAX_BUDDY_ALLOC_SIZE, \
			 PROT_READ | PROT_WRITE, MAP_PRIVATE);
		ASSERT_GE((unsigned long)c_array[i], 0);
		c_array[i][0] = 'a';
		//printf("%p\n", c_array[i]);
	}

	for(i = 0; i < 10; i++) {
		for (j = i+1; j < 10; j++) {
			ASSERT_NE(c_array[i], c_array[j]);
		}
	}

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}
TEST(po_extend, extend_with_PROT_NONE)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_NONE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);
	// cause error
	// printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, extend_with_MAP_ANONYMOUS)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c;
	int i;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, \
		MAP_PRIVATE | MAP_ANONYMOUS);
	ASSERT_GE((unsigned long)c, 0);
	for (i = 0; i < 4096; i++) {
		// printf("%d ", c[i]);
		ASSERT_EQ(c[i], 0);
	}

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, hugepage_simple_test)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c;
	long i;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, MAX_BUDDY_ALLOC_SIZE*2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_HUGETLB);
	ASSERT_GE((unsigned long)c, 0);
	// c[0] = 'a';
	for (i=0; i<MAX_BUDDY_ALLOC_SIZE*2; i++) {
		//printf("i: %ld, &i: %#lx\n", i, &(c[i]));
		c[i] = i;
		//printf("%c\n", c[i]);
	}
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, numa_aware_simple_test)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c;
	long i;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, MAX_BUDDY_ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_NUMA_AWARE);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'a';
	for (i=0; i<MAX_BUDDY_ALLOC_SIZE; i++) {
		c[i] = i;
	}
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_extend, numa_aware_and_hugepage_simple_test)
{
	char poname[] = "f";
	int pod1;
	long long retval1;
	char *c;
	long i;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, MAX_BUDDY_ALLOC_SIZE*4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_NUMA_AWARE | MAP_HUGETLB);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'a';
	for (i=0; i<MAX_BUDDY_ALLOC_SIZE*4; i++) {
		c[i] = i;
	}
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}
