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
