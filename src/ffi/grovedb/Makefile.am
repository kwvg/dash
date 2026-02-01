ACLOCAL_AMFLAGS = -I build-aux/m4

lib_LTLIBRARIES = libgrovedb.la

include_grovedir = $(includedir)/grovedb
include_grove_HEADERS = \
	include/grovedb/cost.h \
	include/grovedb/db.h \
	include/grovedb/element.h \
	include/grovedb/query.h \
	include/grovedb/status.h \
	include/grovedb/transaction.h \
	include/grovedb/types.h

libgrovedb_la_SOURCES = \
	src/db.cpp \
	src/element.cpp \
	src/query.cpp \
	src/transaction.cpp
libgrovedb_la_CPPFLAGS = -DHAVE_GROVEDB_CONFIG -I$(top_srcdir)/include -I$(top_srcdir)/src -I$(top_builddir)/src $(GROVEDB_CXX_CPPFLAGS)
libgrovedb_la_CXXFLAGS = $(AM_CXXFLAGS)

if USE_TESTS
include Makefile.test.include
endif

EXTRA_DIST = LICENSE README.md
