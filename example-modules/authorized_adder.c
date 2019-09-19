/*
 * To explain how to write unittest(ut) and module test(mt),
 * we take this simple adder as an example.
 */

#ifndef __EXAMPLE_MODULES_AUTHORIZED_ADDER_H__
#include "example-modules/authorized_adder.h"
#endif

#ifndef __EXAMPLE_MODULES_AUTHENTICATION_H__
#include "example-modules/authentication.h"
#endif

/*
 * We will use the pin to check whether the user is authorized,
 * where the pin is similar to password, which involves another
 * module, yes, and it is the authentication module.
 */
int authorized_add(int pin, int a, int b)
{
	return is_authorized(pin) ? a + b : -1;
}
