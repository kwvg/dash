AC_PREREQ([2.60])
AC_INIT([libgrovedb], [1.0.0], [https://github.com/dashpay/dash/issues], [libgrovedb], [https://github.com/dashpay/dash])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.11.2 foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AM_PROG_AR
LT_INIT([pic-only])

AX_CXX_COMPILE_STDCXX([20], [noext], [mandatory])

AC_ARG_VAR([GROVEDB_CXX_CPPFLAGS], [C preprocessor flags for grovedb_cxx FFI headers])
AC_ARG_VAR([GROVEDB_CXX_LIBS], [Linker flags for Rust/CXX bridge libraries needed by test binary])

AC_ARG_ENABLE([tests],
  [AS_HELP_STRING([--disable-tests], [do not compile tests (default is to compile)])],
  [use_tests=$enableval],
  [use_tests=yes])

if test "x$use_tests" = "xyes"; then
  AX_BOOST_BASE([1.73.0], [], [
    AC_MSG_WARN([Boost not found; tests will not be built])
    use_tests=no
  ])
fi

AM_CONDITIONAL([USE_TESTS], [test "x$use_tests" = "xyes"])

AC_CONFIG_HEADERS([src/config/grovedb-config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
