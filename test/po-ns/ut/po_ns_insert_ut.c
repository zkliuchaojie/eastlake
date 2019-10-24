#include "ctest/ctest.h"

#include "include/linux/po_metadata.h"

TEST(po_ns_insert, simple_test)
{
	struct po_super *p;
	struct po_ns_record *rcd;

	po_super_init();
	p = po_get_super();
	rcd = po_ns_insert("test", 4);	
	ASSERT_NE(rcd, NULL);
}
