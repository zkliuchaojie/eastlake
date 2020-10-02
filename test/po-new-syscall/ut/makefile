.PHONY: $(OBJDIR)/po_map_area_start_addr_ut $(OBJDIR)/po_creat_ut \
	$(OBJDIR)/po_unlink_ut $(OBJDIR)/po_open_ut \
	$(OBJDIR)/po_close_ut $(OBJDIR)/po_chunk_mmap_ut \
	$(OBJDIR)/po_chunk_munmap_ut $(OBJDIR)/po_extend_ut \
	$(OBJDIR)/po_shrink_ut $(OBJDIR)/po_stat_ut \
	$(OBJDIR)/po_chunk_next_ut $(OBJDIR)/debugger_ut

all-ut: $(OBJDIR)/po_map_area_start_addr_ut $(OBJDIR)/po_creat_ut \
	$(OBJDIR)/po_unlink_ut $(OBJDIR)/po_open_ut \
	$(OBJDIR)/po_close_ut $(OBJDIR)/po_chunk_mmap_ut \
	$(OBJDIR)/po_chunk_munmap_ut $(OBJDIR)/po_extend_ut \
	$(OBJDIR)/po_shrink_ut $(OBJDIR)/po_stat_ut \
	$(OBJDIR)/po_chunk_next_ut $(OBJDIR)/debugger_ut

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

$(OBJDIR)/po_close_ut.o: $(UTDIR)/po_close_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_close_ut: $(OBJDIR)/po_close_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_close_ut

$(OBJDIR)/po_chunk_mmap_ut.o: $(UTDIR)/po_chunk_mmap_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_chunk_mmap_ut: $(OBJDIR)/po_chunk_mmap_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_chunk_mmap_ut

$(OBJDIR)/po_chunk_munmap_ut.o: $(UTDIR)/po_chunk_munmap_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_chunk_munmap_ut: $(OBJDIR)/po_chunk_munmap_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_chunk_munmap_ut

$(OBJDIR)/po_extend_ut.o: $(UTDIR)/po_extend_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_extend_ut: $(OBJDIR)/po_extend_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_extend_ut

$(OBJDIR)/po_shrink_ut.o: $(UTDIR)/po_shrink_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_shrink_ut: $(OBJDIR)/po_shrink_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_shrink_ut

$(OBJDIR)/po_stat_ut.o: $(UTDIR)/po_stat_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_stat_ut: $(OBJDIR)/po_stat_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_stat_ut

$(OBJDIR)/po_chunk_next_ut.o: $(UTDIR)/po_chunk_next_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/po_chunk_next_ut: $(OBJDIR)/po_chunk_next_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/po_chunk_next_ut

$(OBJDIR)/debugger_ut.o: $(UTDIR)/debugger_ut.c
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR)/debugger_ut: $(OBJDIR)/debugger_ut.o \
	$(OBJDIR)/ctest_main.a
	$(CC) $(CFLAGS) -lpthread $^ -o $@
	./$(OBJDIR)/debugger_ut
# next ut for a file, it's yours
