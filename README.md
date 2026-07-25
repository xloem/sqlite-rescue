# sqlite-rescue

Speedy database recovery. Unflush your SQLite data!


This simple developer tool processes streams from arbitrarily damaged media and extracts any intact SQLite database rows that pass.


The user must write or generate, and compile, short parsing functions that describe their database columns and what to do with their data.

See the example tool scan-db-guix.c, which recovers Guix store database rows, or sqlite.h for just the interface.

Note, this is simple, basic code. Small improvements, such as adding multithreading, could make it even faster.
