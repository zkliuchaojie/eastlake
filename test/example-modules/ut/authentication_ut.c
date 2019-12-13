#include "ctest/ctest.h"

#ifndef __EXAMPLE_MODULE_H__
#include "example-modules/authentication.h"
#endif

TEST(is_authorized, simple_test)
{
	int pin_1 = 1;
	int result_1 = 1;
	int pin_2 = -1;
	int result_2 = 0;

	ASSERT_EQ(is_authorized(pin_1), result_1);
	ASSERT_EQ(is_authorized(pin_2), result_2);
}
