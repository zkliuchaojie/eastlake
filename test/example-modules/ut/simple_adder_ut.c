#include "gtest/gtest.h"

#ifndef __EXAMPLE_MODULES_SIMPLE_ADDER_H__
#include "example-modules/simple_adder.h"
#endif

TEST(simple_add, simple_test)
{
	int a = 1;
	int b = 1;
	int result = 2;

	ASSERT_EQ(simple_add(a, b), result);
}
