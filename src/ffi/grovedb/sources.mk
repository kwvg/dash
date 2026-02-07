# - All Variables ending in _HEADERS or _SOURCES confuse automake, so the
#     _INT postfix is applied.
# - Convenience variables should not be used as they interfere with automatic
#     dependency generation
# - The %reldir% is the relative path from the Makefile.am.

GROVEDB_INCLUDE_DIR_INT = %reldir%/include

GROVEDB_DIST_HEADERS_INT = %reldir%/include/grovedb/vendor/tl/expected.hpp
GROVEDB_DIST_HEADERS_INT += \
	%reldir%/include/grovedb/cost.h \
	%reldir%/include/grovedb/costed.h \
	%reldir%/include/grovedb/db.h \
	%reldir%/include/grovedb/element.h \
	%reldir%/include/grovedb/error.h \
	%reldir%/include/grovedb/query.h \
	%reldir%/include/grovedb/result.h \
	%reldir%/include/grovedb/transaction.h \
	%reldir%/include/grovedb/types.h \
	%reldir%/include/grovedb/wire.h

GROVEDB_LIB_SOURCES_INT = \
	%reldir%/src/db.cpp \
	%reldir%/src/delete.cpp \
	%reldir%/src/element.cpp \
	%reldir%/src/get.cpp \
	%reldir%/src/proof.cpp \
	%reldir%/src/put.cpp \
	%reldir%/src/query.cpp \
	%reldir%/src/transaction.cpp \
	%reldir%/src/types.cpp \
	%reldir%/src/wire_read.cpp \
	%reldir%/src/wire_write.cpp

GROVEDB_TEST_SOURCES_INT = \
	%reldir%/src/test/main.cpp \
	%reldir%/src/test/delete_tests.cpp \
	%reldir%/src/test/element_tests.cpp \
	%reldir%/src/test/get_tests.cpp \
	%reldir%/src/test/insert_tests.cpp \
	%reldir%/src/test/lifecycle_tests.cpp \
	%reldir%/src/test/proof_tests.cpp \
	%reldir%/src/test/query_tests.cpp \
	%reldir%/src/test/transaction_tests.cpp \
	%reldir%/src/test/util/tempdir.cpp \
	%reldir%/src/test/wire_tests.cpp
