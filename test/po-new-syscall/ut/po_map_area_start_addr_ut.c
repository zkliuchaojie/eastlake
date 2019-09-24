#include "gtest/gtest.h"

#ifndef _LINUX_PO_MAP_H
#include "include/linux/po_map.h"
#endif

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

TEST(po_map_area_start_addr, map_a_file_and_check)
{
	char *addr;
	int fd;
	struct stat sb;
	off_t offset, pa_offset;
	size_t length;
	ssize_t s;

	fd = open("./makefile", O_RDONLY);
	if (fd == -1)
		exit(1);

	if (fstat(fd, &sb) == -1)           /* To obtain file size */
		exit(1);

	offset = 0;
	pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
	/* offset for mmap() must be page aligned */

	if (offset >= sb.st_size) {
		fprintf(stderr, "offset is past end of file\n");
		exit(EXIT_FAILURE);
	}

	length = sb.st_size - offset;

	addr = (char *)mmap(NULL, length + offset - pa_offset,
		PROT_READ, MAP_PRIVATE, fd, pa_offset);
	/*
	 * printf("addr: %#lx, PO_MAP_AREA_START: %#lx\n", \
	 *	(unsigned long)addr, PO_MAP_AREA_START);
	 */
	ASSERT_LE((unsigned long)addr, PO_MAP_AREA_START);
}
