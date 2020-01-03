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

TEST(po_mmap, simple_test)
{
	char poname[] = "m";
	int pod1;
	long long retval1;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'a';
	//printf("%c\n", c[0]);

	c = (char *)syscall(404, 0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, pod1, 0);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'b';
	//printf("%c\n", c[0]);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

/*
 * need to be automical.
 */

TEST(po_mmap, just_can_read_test)
{
	char poname[] = "m";
	int pod1;
	long long retval1, retval2;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);

	c = (char *)syscall(404, 0, 4096, PROT_READ, MAP_PRIVATE, pod1, 0);
	ASSERT_GE(retval1, 0);
	//c[0] = 'a';

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}

TEST(po_mmap, close_is_okay_after_mapping_test)
{
	char poname[] = "m";
	int pod1;
	long long retval1, retval2;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);

	c = (char *)syscall(404, 0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, pod1, 0);
	ASSERT_GE(retval1, 0);
	c[0] = 'a';

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	c[0] = 'a';

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}
/*
 * TODO: should be automatic.
 */
TEST(po_mmap, can_not_access_a_mapping_after_unlink__test)
{
	char poname[] = "m";
	int pod1;
	long long retval1;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	c = (char *)syscall(406, pod1, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'a';

	c = (char *)syscall(404, 0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, pod1, 0);
	ASSERT_GE((unsigned long)c, 0);
	c[0] = 'b';

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);

	//printf("%c\n", c[0]);
}

TEST(po_mmap, map_po_after_many_po_extend)
{
	char poname[] = "m";
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

	for (i = 0; i < 10; i++) {
		c_array[i] = (char *)syscall(404, 0, 4096, PROT_READ | PROT_WRITE, \
			MAP_PRIVATE, pod1, i*2*MAX_BUDDY_ALLOC_SIZE);
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

TEST(po_mmap, mmap_with_PROT_NONE)
{
	char poname[] = "m";
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
