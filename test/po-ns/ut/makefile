.PHONY: $(OBJDIR)/po_ns_insert_ut

all-ut: $(OBJDIR)/po_ns_insert_ut

# an example of ut for simple_adder.c
$(OBJDIR)/po_metadata.o: $(ROOTDIR)/mm/po_metadata.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_ns_insert_ut.o: $(UTDIR)/po_ns_insert_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_ns_insert_ut: $(OBJDIR)/po_ns_insert_ut.o $(OBJDIR)/po_metadata.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_ns_insert_ut

# next ut for a file, it's yours
