.PHONY: $(OBJDIR)/simple_adder_ut $(OBJDIR)/authentication_ut

all-ut: $(OBJDIR)/simple_adder_ut $(OBJDIR)/authentication_ut

# an example of ut for simple_adder.c
$(OBJDIR)/simple_adder.o: $(ROOTDIR)/example-modules/simple_adder.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/simple_adder_ut.o: $(UTDIR)/simple_adder_ut.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/simple_adder_ut: $(OBJDIR)/simple_adder_ut.o $(OBJDIR)/simple_adder.o \
	$(OBJDIR)/gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/simple_adder_ut

# an example of ut for authentication.c
$(OBJDIR)/authentication.o: $(ROOTDIR)/example-modules/authentication.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/authentication_ut.o: $(UTDIR)/authentication_ut.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/authentication_ut: $(OBJDIR)/authentication_ut.o $(OBJDIR)/authentication.o \
	$(OBJDIR)/gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/authentication_ut


# next ut for a file, it's yours
