
AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libklbars.la

libklbars_la_SOURCES = klbars.c klbars-tone.c klbars-char.c klbars-eia189.c klbars-black.c klbars-rp219-1.c klbars-rp198.c
libklbars_la_SOURCES += klbars-internal.h

noinst_HEADERS = font8x8_basic.h

libklbars_la_CFLAGS = -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" \
	-D_FILE_OFFSET_BITS=64 -O3 -D_BSD_SOURCE -I$(top_srcdir)/include \
	-DKL_USERSPACE

if DEBUG
  libklbars_la_CFLAGS += -g
endif

libklbars_includedir = $(includedir)/libklbars

libklbars_include_HEADERS  = libklbars/klbars.h
