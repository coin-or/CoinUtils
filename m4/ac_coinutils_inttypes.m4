AC_DEFUN([AC_COINUTILS_INTTYPE_HDRS],
[
  #ifdef HAVE_CSTDINT
  # include <cstdint>
  #else
  # ifdef HAVE_STDINT_H
  #  include <stdint.h>
  # endif
  #endif
])


# Find working type names for signed and unsigned 64-bit values. `int64_t' is
# preferred if it exists; otherwise the macro will fall back to 'long long',
# 'long', and finally 'int'. There are two preprocessor defines because it's
# not legal to decorate int64_t with `unsigned'.

AC_DEFUN([AC_COINUTILS_INT64],
[
# stdint.h is part of autoconf's default standard includes and is checked very
# early on, as part of libtool setup. CHECK_HEADERS will stop if it finds
# cstdint, but it's possible (indeed, likely) for both ac_cv_header_cstdint
# and ac_cv_header_stdint_h to be defined.

# 171102 lh Why do we need COINUTILS_HAS_CSTDINT and COINUTILS_HAS_STDINT_H?
#        Seems like HAVE_CSTDINT and HAVE_STDINT_H should suffice. These
#        symbols are exported in config_coinutils.h.

  AC_CHECK_HEADERS([cstdint stdint.h],[break],[])
  if test "x$ac_cv_header_cstdint" = xyes ; then
    AC_DEFINE([COINUTILS_HAS_CSTDINT],[1],
              [Define to 1 if cstdint is available for CoinUtils])
  fi
  if test "x$ac_cv_header_stdint_h" = xyes ; then
    AC_DEFINE([COINUTILS_HAS_STDINT_H],[1],
              [Define to 1 if stdint.h is available for CoinUtils])
  fi

  CoinInt64=
  CoinUInt64=

  # Try int64_t. Assume uint64_t exists if int64_t exists.

  AC_CHECK_TYPE([int64_t],[CoinInt64=int64_t ; CoinUInt64=uint64_t],[],
                AC_COINUTILS_INTTYPE_HDRS)

# We need to use the C compiler in CHECK_SIZEOF. Otherwise, the MSVC compiler
# complains about redefinition of "exit". ac_cv_sizeof_<type> sometimes adds
# `^M' to the number, hence the check for `8?'.

  if test -z "$CoinInt64" ; then
    AC_LANG_PUSH(C)
    AC_CHECK_SIZEOF([long long])
    case $ac_cv_sizeof_long_long in
      8 | 8?) CoinInt64="long long"
              CoinUInt64="unsigned long long"
              ;;
      esac

    if test -z "$CoinInt64" ; then
      AC_CHECK_SIZEOF([long])
      case $ac_cv_sizeof_long in
        8 | 8?) CoinInt64="long"
                CoinUInt64="unsigned long"
                ;;
      esac
    fi

    if test -z "$CoinInt64" ; then
      AC_CHECK_SIZEOF([int])
      case $ac_cv_sizeof_int in
        8 | 8?) CoinInt64="int"
                CoinUInt64="unsigned int"
                ;;
      esac
    fi
    AC_LANG_POP(C)
  fi

  if test -z "$CoinInt64" ; then
    AC_MSG_ERROR([Cannot find 64-bit integer type.])
  fi
  AC_DEFINE_UNQUOTED([COINUTILS_INT64_T], [$CoinInt64],
                     [Define to 64-bit integer type])
  AC_DEFINE_UNQUOTED([COINUTILS_UINT64_T],[$CoinUInt64],
                     [Define to 64-bit unsigned integer type])
])


##### Integer type for Pointer
# Much as above. Look for intptr_t first, then fall back to long long, long,
# and int.

AC_DEFUN([AC_COINUTILS_INTPTR],
[
  CoinIntPtr=

  # Try intptr_t, preferred if we can get it.

  AC_CHECK_TYPE([intptr_t],[CoinIntPtr=intptr_t],[],AC_COINUTILS_INTTYPE_HDRS)

  if test -z "$CoinIntPtr" ; then
    AC_LANG_PUSH(C)
    AC_CHECK_SIZEOF([int *])
    AC_CHECK_SIZEOF([long long])
    if test "$ac_cv_sizeof_long_long" = "$ac_cv_sizeof_int_p" ; then
      CoinIntPtr="long long"
    fi

    if test x"$CoinIntPtr" = x ; then
      AC_CHECK_SIZEOF([long])
      if test "$ac_cv_sizeof_long" = "$ac_cv_sizeof_int_p" ; then
        CoinIntPtr="long"
      fi
    fi

    if test x"$CoinIntPtr" = x ; then
      AC_CHECK_SIZEOF([int])
      if test "$ac_cv_sizeof_int" = "$ac_cv_sizeof_int_p" ; then
        CoinIntPtr="int"
      fi
    fi
    AC_LANG_POP(C)
  fi

  if test -z "$CoinIntPtr" ; then
    AC_MSG_ERROR([Cannot find integer type capturing pointer])
  fi
  AC_DEFINE_UNQUOTED([COINUTILS_INTPTR_T],[$CoinIntPtr],[Define to integer type capturing pointer])
])

AC_DEFUN([AC_COINUTILS_BIGTYPES],
[
  AC_MSG_CHECKING([type to use for CoinBigIndex])
  AC_ARG_ENABLE([coinutils-bigindex],
    [AS_HELP_STRING([--enable-coinutils-bigindex],[use a long integer type for CoinBigIndex])],
    [if test "$enableval" = yes ; then
       bigindextype="long long"
     else
       bigindextype="$enableval"
     fi],
    [bigindextype=int])
  AC_MSG_RESULT([$bigindextype])

  AC_CHECK_TYPE([$bigindextype],[],[AC_MSG_ERROR([Type $bigindextype not available])],AC_COINUTILS_INTTYPE_HDRS)

  AC_DEFINE_UNQUOTED([COINUTILS_BIGINDEX_T],[$bigindextype],[Define to type of CoinBigIndex])
  
  # Osl- and SimpFactorization can only build with BigIndexType int
  AM_CONDITIONAL([BUILD_OSLFACTORIZATION], test "$bigindextype" = int)
  
  if test "$bigindextype" = int ; then
    AC_DEFINE([COINUTILS_BIGINDEX_IS_INT],[1],[Define to 1 if CoinBigIndex is int])
  fi
])
