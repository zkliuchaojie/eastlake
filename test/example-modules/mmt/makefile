.PHONY: $(OBJDIR)/authorized_adder_ut

all-mmt: $(OBJDIR)/authorized_adder_ut

# an example of mmt for authorized_adder.c
$(OBJDIR)/authorized_adder.o: $(ROOTDIR)/example-modules/authorized_adder.c
	$(CC) $(CFLAGS) $(CFLAGS) -c $< -o $@
$(OBJDIR)/authorized_adder_ut.o: $(MMTDIR)/authorized_adder_ut.c
	$(CC) $(CFLAGS) $(CFLAGS) -c $< -o $@
$(OBJDIR)/authorized_adder_ut: $(OBJDIR)/authorized_adder_ut.o $(OBJDIR)/authorized_adder.o \
	$(OBJDIR)/authentication.o $(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/authorized_adder_ut

# next mmt for a file, it's yours
