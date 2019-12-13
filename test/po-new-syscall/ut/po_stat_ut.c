/*
 * Author: liuchaojie
 */


#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include "ctest/ctest.h"

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sys/types.h>
#include <sys/stat.h>

struct po_stat {
	mode_t  st_mode;
	uid_t   st_uid;
	gid_t   st_gid;
	off_t   st_size;
};

TEST(po_stat, simple_test)
{
	char poname[] = "s";
	int pod;
	struct po_stat stat;
	unsigned long retval;

	pod = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod, 0);

	retval = syscall(408, poname, &stat);
	ASSERT_EQ(retval, 0);
	
	ASSERT_EQ(stat.st_size, 0);

	pod = syscall(403, pod, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname, 0);
	ASSERT_EQ(pod, 0);
}

TEST(po_fstat, simple_test)
{
	char poname[] = "s";
	int pod;
	struct po_stat stat;
	unsigned long retval;

	pod = syscall(402, poname, O_CREAT|O_RDWR);
	ASSERT_GE(pod, 0);

	retval = syscall(409, pod, &stat);
	ASSERT_EQ(retval, 0);

	ASSERT_EQ(stat.st_size, 0);
	
	pod = syscall(403, pod, 0);
	ASSERT_GE(pod, 0);

	pod = syscall(401, poname, 0);
	ASSERT_EQ(pod, 0);
}
