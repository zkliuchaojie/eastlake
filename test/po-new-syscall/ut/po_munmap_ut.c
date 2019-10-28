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

TEST(po_munmap, simple_test)
{
	char poname[] = "u";
	int pod1, pod2;
	long long retval1, retval2;
	char *c;

	pod1 = syscall(400, poname);
	ASSERT_GE(pod1, 0);

	pod2 = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod2, 0);

	c = (char *)syscall(404, 0, 4096, PROT_WRITE, MAP_PRIVATE, pod2, 0);
	ASSERT_GE(retval1, 0);
	c[0] = 'a';
	//printf("%c\n", c[0]);
	retval1 = syscall(405, c, 4096);
	ASSERT_EQ(retval1, 0);

	retval1 = syscall(403, pod1, 0);
	ASSERT_GE(retval1, 0);

	retval2 = syscall(403, pod2, 0);
	ASSERT_GE(retval2, 0);

	retval1 = syscall(401, poname, 0);
	ASSERT_EQ(retval1, 0);
}
