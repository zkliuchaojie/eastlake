.PHONY: $(OBJDIR)/authorized_adder_ut

all-mmt: $(OBJDIR)/authorized_adder_ut

# an example of mmt for authorized_adder.c
$(OBJDIR)/authorized_adder.o: $(ROOTDIR)/example-modules/authorized_adder.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/authorized_adder_ut.o: $(MMTDIR)/authorized_adder_ut.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/authorized_adder_ut: $(OBJDIR)/authorized_adder_ut.o $(OBJDIR)/authorized_adder.o \
	$(OBJDIR)/authentication.o $(OBJDIR)/gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/authorized_adder_ut

# next mmt for a file, it's yours
