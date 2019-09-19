#ifndef __EXAMPLE_MODULES_AUTHENTICATION_H__
#define __EXAMPLE_MODULES_AUTHENTICATION_H__

#ifdef GOOGLE_MOCK
// to mock this module, we must create a class for this module.
#include "gmock/gmock.h"

class AuthenticationInterface {
public:
	virtual ~AuthenticationInterface() { }
	virtual int is_authorized_mock(int pin) = 0;
};

class MockAuthentication : public AuthenticationInterface {
public:
	MOCK_METHOD1(is_authorized_mock, int(int pin));
};
// the definition of auth is in the test file.
extern AuthenticationInterface *auth;
// using macro to transfer c to c++.
#define is_authorized(pin) (auth->is_authorized_mock(pin))

#else

int is_authorized(int pin);

#endif

#endif
