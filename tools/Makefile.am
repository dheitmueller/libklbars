
AUTOMAKE_OPTIONS = foreign

CFLAGS += -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 -O3 \
	-I$(top_srcdir)/src

LDFLAGS += -L$(top_builddir)/src/.libs -lpthread -lklbars

if DEBUG
	CFLAGS += -g
endif

CXXFLAGS = $(CFLAGS)

SRC  = klbars-test.c

bin_PROGRAMS  = klbars_test

klbars_test_SOURCES = $(SRC)

libklbars_noinst_includedir = $(includedir)
