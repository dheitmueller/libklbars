
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 \
	-I$(top_srcdir)/src
AM_CFLAGS = -Wall -O3

LDADD = ../src/libklbars.la -lpthread

if DEBUG
	CFLAGS += -g
endif

CXXFLAGS = $(CFLAGS)

klbars_test_SRC  = klbars-test.c
klbars_benchmark_SRC  = klbars-benchmark.c

bin_PROGRAMS  = klbars_test klbars_benchmark

klbars_test_SOURCES = $(klbars_test_SRC)
klbars_benchmark_SOURCES = $(klbars_benchmark_SRC)

libklbars_noinst_includedir = $(includedir)
