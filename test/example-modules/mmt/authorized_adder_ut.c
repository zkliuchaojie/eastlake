#include "ctest/ctest.h"

#ifndef __EXAMPLE_MODULES_AUTHORIZED_ADDER_H__
#include "example-modules/authorized_adder.h"
#endif

TEST(authorized_add, simple_test)
{
	int a = 1;
	int b = 1;

	ASSERT_EQ(authorized_add(1, a, b), a + b);
	ASSERT_EQ(authorized_add(-1, a, b), -1);
}
