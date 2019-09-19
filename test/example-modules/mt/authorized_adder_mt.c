#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef __EXAMPLE_MODULES_AUTHORIZED_ADDER_H__
#include "example-modules/authorized_adder.h"
#endif

#ifndef __EXAMPLE_MODULES_AUTHENTICATION_H__
#include "example-modules/authentication.h"
#endif

using ::testing::Return;

AuthenticationInterface *auth;

TEST(authorized_add_mock, simple_test)
{
	int a = 1;
	int b = 1;
	int pin_1 = 1;
	int pin_2 = -1;

	MockAuthentication mock_auth;
	EXPECT_CALL(mock_auth, is_authorized_mock(pin_1))
		.WillOnce(Return(1));
	EXPECT_CALL(mock_auth, is_authorized_mock(pin_2))
		.WillOnce(Return(0));
	auth = &mock_auth;

	ASSERT_EQ(authorized_add(pin_1, a, b), a + b);
	ASSERT_EQ(authorized_add(pin_2, a, b), -1);
}
