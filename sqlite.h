#pragma once

#include <stddef.h>
#include <stdint.h>
struct sqlite_parse_t;


// user callback
// should return 1 if parse succeeds and 0 if it fails
// use the parse_sqlite_* grouped functions below to parse the record
// note i think autoincrement columns are null
typedef int (*sqlite_record_parser)(struct sqlite_parse_t * parse, uint32_t rowid, void * userdata);


// primary function
// returns a pointer to the end of the record if data parsed, or 0 if it didn't
uint8_t const * parse_sqlite_record(uint8_t const * data, size_t datalen, sqlite_record_parser parser, void * userdata);


// use these to implement sqlite_record_parser
// they return 1 on success and 0 on failure, writing the parsed value into their arguments
// parse_sqlite_null() does not touch the cursor if it fails
int parse_sqlite_null(struct sqlite_parse_t * parse, void ** null/* = 0*/);
int parse_sqlite_int(struct sqlite_parse_t * parse, uint64_t * integer);
int parse_sqlite_float(struct sqlite_parse_t * parse, double * floating);
int parse_sqlite_text(struct sqlite_parse_t * parse, char const ** text, uint32_t * text_len);
int parse_sqlite_blob(struct sqlite_parse_t * parse, uint8_t const ** blob, uint32_t * blob_len);
int parse_sqlite_done(struct sqlite_parse_t * parse);


// search functions
// these return number of records found, or < 0 for a failed errno
// set *maxsize if nonzero, for example to 0 bytes. it will be increased when a larger record is found.
size_t sqlite_search_mem(void const * buf, size_t buflen, size_t * maxsize/* = 0*/, sqlite_record_parser parser, void * userdata);
int64_t sqlite_search_fd(int fd, size_t chunk, sqlite_record_parser parser, void * userdata);


// types
struct sqlite_cursor_t {
	uint8_t const * hdrdata; // working pointer into the columns header
	uint8_t const * data; // working pointer into the columns data
};
struct sqlite_parse_t {
	uint8_t const * const recptr; // start of the record
	uint8_t const * const hdrptr; // start of the columns header
	uint8_t const * const dataptr; // start of the columns data
	uint8_t const * const rectail; // end of the record

	// cache and restore pos to move around
	struct sqlite_cursor_t pos;
};
