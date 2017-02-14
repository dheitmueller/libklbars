
AUTOMAKE_OPTIONS = foreign

CFLAGS += -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 -O3 \
	-I$(top_srcdir)/src

LDFLAGS += -L$(top_builddir)/src/.libs -lpthread -lklbars

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
