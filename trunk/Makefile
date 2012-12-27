MEXLINKFLAGS = -lsqlite3
MEXSUFFIX = .mexa64

default: all

.SUFFIXES: $(MEXSUFFIX) .c .o

.c.o:
	mex -c $<

.c$(MEXSUFFIX):
	mex $< $(MEXLINKFLAGS)

sqlite4m.o: sqlite4m.c sqlite4m.h

libsqlite4m.so: sqlite4m.o
	gcc -shared -o $@ $^

sql_insert$(MEXSUFFIX): sql_insert.c sqlite4m.h
sql_select_numeric$(MEXSUFFIX): sql_select_numeric.c sqlite4m.h
	mex $< $(MEXLINKFLAGS) -L. -lsqlite4m
sql_select$(MEXSUFFIX): sql_select.c sqlite4m.h
	mex $< $(MEXLINKFLAGS) -L. -lsqlite4m
sql_stmt(MEXSUFFIX): sql_stmt.c sqlite4m.h
sql_exec(MEXSUFFIX): sql_exec.c sqlite4m.h

all: sql_open$(MEXSUFFIX) sql_close$(MEXSUFFIX) sql_insert$(MEXSUFFIX) \
	sql_select_numeric$(MEXSUFFIX) sql_select$(MEXSUFFIX) \
	sql_db_filename$(MEXSUFFIX) \
	sql_stmt$(MEXSUFFIX) sql_exec$(MEXSUFFIX) sql_tables$(MEXSUFFIX)

sqlite4m_source.zip: sqlite4m.h sqlite4m.c sql_columnnames.m sql_tables.c \
	sql_open.c sql_close.c sql_open.m sql_close.m \
	sql_insert.c sql_insert.m \
	sql_exec.c sql_stmt.c \
	Makefile sqlite4m_tests.m
	zip sqlite4m_source.zip $^
