CTEST_DIR = ../test-framework/ctest/ctest
# CMOCK_DIR = ../test-framework/cmock/cmock

CFLAGS += -isystem $(CTEST_DIR)/include
# CFLAGS += -isystem $(CTEST_DIR)/include -isystem $(CMOCK_DIR)/include

CTEST_HEADERS = $(CTEST_DIR)/include/gtest/*.h $(CTEST_DIR)/include/gtest/internal/*.h
# CMOCK_HEADERS = $(CMOCK_DIR)/include/cmock/*.h $(CMOCK_DIR)/include/cmock/internal/*.h

all : $(OBJDIR)/ctest_main.a
# all : $(OBJDIR)/ctest_main.a $(OBJDIR)/cmock_main.a

CTEST_SRCS_ = $(CTEST_DIR)/src/*.c
# CMOCK_SRCS_ = $(CMOCK_DIR)/src/*.c

# for cmock_main.a
$(OBJDIR)/ctest_all.o : $(CTEST_SRCS_)
	$(CC) $(CFLAGS) -I$(CTEST_DIR) -c $(CTEST_DIR)/src/ctest_all.c -o $@

$(OBJDIR)/ctest_main.o : $(CTEST_SRCS_)
	$(CC) $(CFLAGS) -I$(CTEST_DIR) -c $(CTEST_DIR)/src/ctest_main.c -o $@

$(OBJDIR)/ctest_main.a : $(OBJDIR)/ctest_all.o $(OBJDIR)/ctest_main.o
	$(AR) $(ARFLAGS) $@ $^

# for cmock_main.a
# $(OBJDIR)/cmock_all.o : $(CMOCK_SRCS_)
# 	$(CC) $(CFLAGS) -I$(CMOCK_DIR) -c $(CMOCK_DIR)/src/cmock_all.c -o $@

# $(OBJDIR)/cmock_main.o : $(CMOCK_SRCS_)
#	$(CC) $(CFLAGS) -I$(CMOCK_DIR) $(CFLAGS) -c $(CMOCK_DIR)/src/cmock_main.c -o $@

# $(OBJDIR)/cmock_main.a : $(OBJDIR)/cmock_all.o $(OBJDIR)/cmock_main.o
#	$(AR) $(ARFLAGS) $@ $^

