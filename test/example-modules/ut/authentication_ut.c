#include "gtest/gtest.h"

// this is a relative directory, the root dir is: ../..
#ifndef __EXAMPLE_MODULE_H__
#include "example-modules/authentication.h"
#endif

TEST(is_authorized, simple_test)
{
	int pin_1 = 1;
	bool result_1 = true;
	int pin_2 = -1;
	bool result_2 = false;

	ASSERT_EQ(is_authorized(pin_1), result_1);
	ASSERT_EQ(is_authorized(pin_2), result_2);
}
