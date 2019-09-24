.PHONY: $(OBJDIR)/po_map_area_start_addr_ut

all-ut: $(OBJDIR)/po_map_area_start_addr_ut

$(OBJDIR)/po_map_area_start_addr_ut.o: $(UTDIR)/po_map_area_start_addr_ut.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/po_map_area_start_addr_ut: $(OBJDIR)/po_map_area_start_addr_ut.o \
	$(OBJDIR)/gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)//po_map_area_start_addr_ut

# next ut for a file, it's yours
