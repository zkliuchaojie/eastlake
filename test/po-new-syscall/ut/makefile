.PHONY: $(OBJDIR)/po_map_area_start_addr_ut $(OBJDIR)/po_creat_ut \
	$(OBJDIR)/po_unlink_ut $(OBJDIR)/po_open_ut

all-ut: $(OBJDIR)/po_map_area_start_addr_ut $(OBJDIR)/po_creat_ut \
	$(OBJDIR)/po_unlink_ut $(OBJDIR)/po_open_ut

$(OBJDIR)/po_map_area_start_addr_ut.o: $(UTDIR)/po_map_area_start_addr_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_map_area_start_addr_ut: $(OBJDIR)/po_map_area_start_addr_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_map_area_start_addr_ut

$(OBJDIR)/po_creat_ut.o: $(UTDIR)/po_creat_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_creat_ut: $(OBJDIR)/po_creat_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_creat_ut

$(OBJDIR)/po_unlink_ut.o: $(UTDIR)/po_unlink_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_unlink_ut: $(OBJDIR)/po_unlink_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_unlink_ut

$(OBJDIR)/po_open_ut.o: $(UTDIR)/po_open_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_open_ut: $(OBJDIR)/po_open_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_open_ut

# next ut for a file, it's yours
