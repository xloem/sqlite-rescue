#include "sqlite.h"

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

size_t sqlite_search_mem(void const * buf, size_t buflen, size_t * maxsize, sqlite_record_parser parser, void * userdata)
{
	uint8_t const * data = buf;
	uint8_t const * tail = data + buflen;
	uint8_t const * next_data;
	size_t finds = 0;
	while (data < tail) {
		next_data = parse_sqlite_record(data, tail - data, parser, userdata);
		if (next_data) {
			if (maxsize)
				if ((size_t)(next_data - data) > *maxsize)
					*maxsize = next_data - data;
			++ finds;
			data = next_data;
			continue;
		}
		if ((size_t)(tail - data) >= sizeof(uint64_t)) {
			if (!*(uint64_t const *)data) {
				data += sizeof(uint64_t);
				continue;
			}
		}
		++ data;
	}
	return finds;
}

int64_t sqlite_search_fd(int fd, size_t chunk, sqlite_record_parser parser, void * userdata)
{
	size_t overlap = 1024;
	size_t kept = 0;
	uint64_t buffer_offset = 0;
	uint64_t next_offset = 0;
	int64_t finds = 0;
	size_t buflen = chunk + overlap;
	unsigned char *buf = malloc(buflen);
	if (!buf) {
		return -errno;
	}

	for (;;) {
		ssize_t nr = read(fd, buf + kept, chunk);
		if (nr <= 0) {
			if (errno == EINTR) {
				continue;
			} else {
				if (nr != 0)
					finds = nr;
				break;
			}
		}

		size_t total = kept + (size_t)nr;
		size_t start = 0;

		if (next_offset > buffer_offset) {
			uint64_t delta =
				next_offset - buffer_offset;

			start = delta < total
				? (size_t)delta
				: total;
		}

		finds += sqlite_search_mem(buf + start, total - start, &overlap, parser, userdata);

		if (total >= overlap) {
			next_offset = buffer_offset + total - overlap + 1;
		}

		kept = total < overlap ? total : overlap;

		if (chunk + overlap > buflen) {
			size_t newbuflen = chunk + overlap;
			unsigned char * newbuf = malloc(newbuflen);
			memcpy(newbuf, buf + total - kept, kept);
			buf = newbuf;
			buflen = newbuflen;
		} else {
			memmove(buf, buf + total - kept, kept);
		}

		buffer_offset += total - kept;
	}

	free(buf);

	return finds;
}
