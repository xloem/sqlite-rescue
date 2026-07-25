#include "sqlite.h"

#include <endian.h>

int parse_varint(uint8_t const ** data, uint8_t const * tail, uint32_t * value)
{
	if (*data >= tail) return 0;
	if (**data == 0x80) return 0;
	*value = **data & 0x7f;
	while (**data & 0x80) {
		if (*value >= 0x02000000) return 0;
		*value <<= 7;
		++ *data;
		if (*data == tail) return 0;
		*value |= (**data & 0x7f);
	}
	++ *data;
	return 1;
}

uint8_t const * parse_sqlite_record(uint8_t const * data, size_t datalen, sqlite_record_parser parser, void * userdata)
{
	uint8_t const * recdata = data;
	uint8_t const * extradata;
	uint8_t const * tail = data + datalen;
	uint32_t reclen, rowid, hdrlen;

	if (!parse_varint(&recdata, tail, &reclen)) return 0;
	if (!parse_varint(&recdata, tail, &rowid)) return 0;
	extradata = recdata;
	if (!parse_varint(&extradata, tail, &hdrlen)) return 0;

	struct sqlite_parse_t parse = {
		.recptr = data,
		.hdrptr = recdata,
		.dataptr = recdata + hdrlen,
		.rectail = recdata + reclen,
		.pos = {
			.hdrdata = extradata,
			.data = recdata + hdrlen
		}
	};

	if (parse.rectail > tail) return 0;
	if (parse.dataptr > parse.rectail) return 0;

	if (!parser(&parse, rowid, userdata)) return 0;

	return parse.rectail;
}


int parse_sqlite_null(struct sqlite_parse_t * parse, void ** null)
{
	if (parse->pos.hdrdata >= parse->dataptr) return 0;
	if (*parse->pos.hdrdata != 0) return 0;
	++ parse->pos.hdrdata;
	if (null) *null = 0;
	return 1;
}

int parse_sqlite_int(struct sqlite_parse_t * parse, uint64_t * integer)
{
	uint32_t type;
	uint8_t int_len;
	if (!parse_varint(&parse->pos.hdrdata, parse->dataptr, &type)) return 0;
	if (type == 0 || type == 8 || type >= 10) return 0;
	if (type < 5) {
		int_len = type;
	} else if (type < 7) {
		int_len = ((uint8_t)type - 2) * 2;
	} else {
		*integer = type & 1;
		return 1;
	}
	uint8_t const * int_tail = parse->pos.data + int_len;
	if (int_tail > parse->rectail) return 0;

	/* the below approach to forming a mask ensures the shift
	 * is never as large as the word size, avoiding undefined
	 * and inconsistent behavior in gcc.
	 */
	*integer = be64toh(*(uint64_t*)(parse->pos.data + int_len - 8))
		& ((~(uint64_t)0) >> (64 - int_len * 8)); 

	parse->pos.data = int_tail;
	return 1;
}

int parse_sqlite_float(struct sqlite_parse_t * parse, double * floating)
{
	if (parse->pos.hdrdata >= parse->dataptr) return 0;
	if (*parse->pos.hdrdata != 7) return 0;
	if (parse->pos.data + 8 > parse->rectail) return 0;
	++ parse->pos.hdrdata;
	*floating = *(double*)parse->pos.data;
	parse->pos.data += 8;
	return 1;
}

int parse_sqlite_text(struct sqlite_parse_t * parse, char const ** text, uint32_t * text_len)
{
	uint32_t type;
	if (!parse_varint(&parse->pos.hdrdata, parse->dataptr, &type)) return 0;
	if (type < 13 || !(type & 1)) return 0;
	*text_len = (type - 13) / 2;
	if ((uint32_t)(parse->rectail - parse->pos.data) < *text_len) return 0;
	*text = (void*)parse->pos.data;
	parse->pos.data += *text_len;
	return 1;
}

int parse_sqlite_blob(struct sqlite_parse_t * parse, uint8_t const ** blob, uint32_t * blob_len)
{
	uint32_t type;
	if (!parse_varint(&parse->pos.hdrdata, parse->dataptr, &type)) return 0;
	if (type < 12 || (type & 1)) return 0;
	*blob_len = (type - 12) / 2;
	if ((uint32_t)(parse->rectail - parse->pos.data) < *blob_len) return 0;
	*blob = parse->pos.data;
	parse->pos.data += *blob_len;
	return 1;
}

int parse_sqlite_done(struct sqlite_parse_t * parse)
{
	return parse->pos.hdrdata == parse->dataptr && parse->pos.data == parse->rectail;
}
