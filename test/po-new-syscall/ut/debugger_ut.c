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

TEST(debugger, simple_test)
{
	int ret;

	ret = syscall(411, 0);
	ASSERT_GE(ret, 0);
	
	ret = syscall(411, 1);
	ASSERT_GE(ret, 1);
}
