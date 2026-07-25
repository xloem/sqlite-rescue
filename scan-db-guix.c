#include "sqlite.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

char const * CREATE_ValidPaths = "CREATE TABLE ValidPaths (\n\
    id               integer primary key autoincrement not null,\n\
    path             text unique not null,\n\
    hash             text not null,\n\
    registrationTime integer not null,\n\
    deriver          text,\n\
    narSize          integer\n\
);\n";
int parse_validpath_record(struct sqlite_parse_t * parse, uint32_t rowid)
{
	char const * path; uint32_t path_len;
	char const * hash; uint32_t hash_len;
	uint64_t registrationTime;
	char const * deriver; uint32_t deriver_len;
	uint64_t narSize;

	// id (autoincrement)
	if (!parse_sqlite_null(parse, 0)) return 0;
	
	// path
	if (!parse_sqlite_text(parse, &path, &path_len)) return 0;

	// hash
	if (!parse_sqlite_text(parse, &hash, &hash_len)) return 0;

	// registrationTime
	if (!parse_sqlite_int(parse, &registrationTime)) return 0;

	// deriver, can be null
	if (parse_sqlite_null(parse, (void**)&deriver)) {
		deriver_len = 0;
	} else if (!parse_sqlite_text(parse, &deriver, &deriver_len)) {
		return 0;
	}

	// narSize, can be null
	if (parse_sqlite_null(parse, 0)) {
		narSize = ~0;
	} else if (!parse_sqlite_int(parse, &narSize)) {
		return 0;
	}

	// assert record end
	if (!parse_sqlite_done(parse)) return 0;

	// guix constraint
	if (strncmp(path, "/gnu/store/", 11)) return 0;

	// rowid used for autoincrement id
	printf("INSERT INTO ValidPaths VALUES(%" PRIu32 ",'%.*s','%.*s',%" PRIu64,
			rowid,
			(int)path_len, path,
			(int)hash_len, hash,
			registrationTime);
	if (deriver) {
		printf(",'%.*s'", deriver_len, deriver);
	} else {
		printf(",NULL");
	}
	if (~narSize) {
		printf(",%" PRIu64, narSize);
	} else {
		printf(",NULL");
	}
	printf(");\n");

	return 1;
}

char const * CREATE_Refs = "CREATE TABLE Refs (\n\
    referrer  integer not null,\n\
    reference integer not null,\n\
    primary key (referrer, reference),\n\
    foreign key (referrer) references ValidPaths(id) on delete cascade,\n\
    foreign key (reference) references ValidPaths(id) on delete restrict\n\
);\n";
int parse_refs_record(struct sqlite_parse_t * parse)
{
	uint64_t referrer, reference;

	// referrer
	if (!parse_sqlite_int(parse, &referrer)) return 0;

	// reference
	if (!parse_sqlite_int(parse, &reference)) return 0;

	// assert record end
	if (!parse_sqlite_done(parse)) return 0;

	// guix constraint
	if (reference > referrer) return 0;

	printf("INSERT INTO Refs VALUES(%" PRIu64 ",%" PRIu64 ");\n",
			referrer,
			reference);

	return 1;
}

char const * CREATE_DerivationOutputs = "CREATE TABLE DerivationOutputs (\n\
    drv  integer not null,\n\
    id   text not null, -- symbolic output id, usually \"out\"\n\
    path text not null,\n\
    primary key (drv, id),\n\
    foreign key (drv) references ValidPaths(id) on delete cascade\n\
);\n";
int parse_derivation_outputs_record(struct sqlite_parse_t * parse)
{
	uint64_t drv;
	char const * id; uint32_t id_len;
	char const * path; uint32_t path_len;

	// drv
	if (!parse_sqlite_int(parse, &drv)) return 0;
	
	// id
	if (!parse_sqlite_text(parse, &id, &id_len)) return 0;

	// path
	if (!parse_sqlite_text(parse, &path, &path_len)) return 0;

	// assert record end
	if (!parse_sqlite_done(parse)) return 0;

	// guix constraint
	if (strncmp(path, "/gnu/store/", 11)) return 0;

	printf("INSERT INTO DerivationOutputs VALUES(%" PRIu64 ",'%.*s','%.*s');\n",
			drv,
			id_len, id,
			path_len, path);

	return 1;
}

int guix_db_record_parser(struct sqlite_parse_t * parse, uint32_t rowid, void * userdata)
{
	(void)userdata;
	struct sqlite_cursor_t pos = parse->pos;
	if (parse_validpath_record(parse, rowid)) return 1;
	parse->pos = pos;
	if (parse_refs_record(parse)) return 1;
	parse->pos = pos;
	if (parse_derivation_outputs_record(parse)) return 1;
	parse->pos = pos;
	return 0;
}

int main(int argc, char const **argv)
{
	int fd = 0;
	int64_t finds;
	if (argc > 2) {
		fprintf(stderr, "usage: %s [DEVICE_OR_FILE]\n", argv[0]);
		return 2;
	}
	if (argc == 2) {
		fd = open(argv[1], O_RDONLY | O_CLOEXEC);
		if (fd < 0) {
			perror(argv[1]);
			return 1;
		}
	} else {
		fprintf(stderr, "Reading /var/guix/db/db.sqlite fragments from stdin ...\n");
	}
#ifdef POSIX_FADV_SEQUENTIAL
	(void)posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

	fprintf(stderr, "%s\n%s\n%s\n",
		CREATE_ValidPaths,
		CREATE_Refs,
		CREATE_DerivationOutputs);

	finds = sqlite_search_fd(fd, 8 * 1024 * 1024, guix_db_record_parser, 0);
	if (finds < 0) {
		perror("sqlite_search_fd");
	}
	return finds;
}
