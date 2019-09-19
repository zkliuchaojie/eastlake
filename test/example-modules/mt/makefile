.PHONY: $(OBJDIR)/authorized_adder_mt

all-mt: $(OBJDIR)/authorized_adder_mt

# an example of mt for authorized_adder.c with mock
# this is a moked authorized_adder with -DGOOGLE_MOCK flag
$(OBJDIR)/authorized_adder_mock.o: $(ROOTDIR)/example-modules/authorized_adder.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGOOGLE_MOCK -c $< -o $@
$(OBJDIR)/authorized_adder_mt.o: $(MTDIR)/authorized_adder_mt.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGOOGLE_MOCK -c $< -o $@
$(OBJDIR)/authorized_adder_mt: $(OBJDIR)/authorized_adder_mt.o \
	$(OBJDIR)/authorized_adder_mock.o \
	$(OBJDIR)/gtest_main.a $(OBJDIR)/gmock_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/authorized_adder_mt
