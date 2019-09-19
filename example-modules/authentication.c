/*
 * To explain how to write mock test, which is mostly used in module test(mt),
 * we take this simple authentication module as an example.
 */

#ifndef __EXAMPLE_MODULES_AUTHENTICATION_H__
#include "example-modules/authentication.h"
#endif

/*
 * If pin is bigger than 0, return 1, which means the pin is valid.
 */
int is_authorized(int pin)
{
	return pin > 0 ? 1 : 0;
}
