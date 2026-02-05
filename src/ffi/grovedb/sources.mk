# - All Variables ending in _HEADERS or _SOURCES confuse automake, so the
#     _INT postfix is applied.
# - Convenience variables should not be used as they interfere with automatic
#     dependency generation
# - The %reldir% is the relative path from the Makefile.am.

GROVEDB_INCLUDE_DIR_INT = %reldir%/include

GROVEDB_DIST_HEADERS_INT = \
	%reldir%/include/grovedb/grovedb.h \
	%reldir%/include/grovedb/misc.h

GROVEDB_LIB_SOURCES_INT = \
	%reldir%/src/misc.cpp

GROVEDB_TEST_SOURCES_INT = \
	%reldir%/src/test/main.cpp \
	%reldir%/src/test/misc_tests.cpp
