# sqlite-rescue

Speedy database recovery. Unflush your SQLite data!


This simple developer tool processes streams from arbitrarily damaged media and extracts any intact SQLite database rows that pass.


The user must write or generate, and compile, short parsing functions that describe their database columns and what to do with their data.

See the example tool scan-db-guix.c, which recovers Guix store database rows, or sqlite.h for just the interface.

Note, this is simple, basic code. Small improvements, such as adding multithreading, could make it even faster.

## Known Potential Bugs

I made this in a day so it may not work on things other
than the Guix database.

One concern is there may be no provision for negative numbers.
The check against 0x80 on line 10 of sqlite.c should possibly
be removed.

I expect that if it does not work for you, though, there are
likely just a couple small things that need fixing for it to
plow through whatever data you have.

_However_ the SQLite format here was made entirely from the Autoconf SQLite
variant source that comes with Guix and the Guix DB. So if other SQLite's have
a different format, it will definitely fail to parse them and need to be
updated for their source or format by looking at the database hexdump or the
writing and reading code. 
