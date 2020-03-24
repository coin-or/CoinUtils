# COINUTILS_MEMPOOL(dflt_size, dflt_global)
# -------------------------------------------------------------------------
# Control the CoinUtils private mempool facility. Off by default.
#   dflt_size ($1) sets the default size (in bytes) for the largest block
#                  that will be allocated from the private mempool; if not
#                  given, default is 4096. A value less than zero disables the
#                  private memory pool.
#   dflt_global ($2) controls whether new and delete methods using the private
#                  mempool will be defined in the global namespace, overriding
#                  new and delete in the std namespace; if not given, the
#                  default is no.
# -------------------------------------------------------------------------

AC_DEFUN([AC_COINUTILS_MEMPOOL],
[
  AC_ARG_ENABLE([coinutils-mempool-maxpooled],
    [AS_HELP_STRING([--enable-coinutils-mempool-maxpooled],
    [Specify the size in bytes of the largest block that will be
     allocated from the CoinUtils mempool. If negative (or 'no') then
     the memory pool is disabled completely. This value can be
     overridden at runtime using the COINUTILS_MEMPOOL_MAXPOOLED
     environment variable.])],
    [coinutils_mempool_maxpooled=$enableval],
    [coinutils_mempool_maxpooled=no])

  AC_ARG_ENABLE([coinutils-mempool-override-new],
      [AS_HELP_STRING([--enable-coinutils-mempool-override-new],
      [Redirects global new/delete to CoinUtils mempool.])],
      [coinutils_mempool_override=$enableval],
      [coinutils_mempool_override=m4_ifvaln([$2],[$2],[no])])

  if test x"$coinutils_mempool_override" = xyes ; then
    AC_DEFINE([COINUTILS_MEMPOOL_OVERRIDE_NEW],[1],
      [Define to 1 if CoinUtils should override global new/delete.])
  fi

  if test x"$coinutils_mempool_maxpooled" = xyes ; then
    coinutils_mempool_maxpooled=m4_ifvaln([$1],[$1],[4096])
  fi
  if test x"$coinutils_mempool_maxpooled" = xno ; then
    coinutils_mempool_maxpooled=-1
    AC_DEFINE([COINUTILS_MEMPOOL_MAXPOOLED],[-1],[Disable CoinUtils memory pool.])
  else
    AC_DEFINE_UNQUOTED([COINUTILS_MEMPOOL_MAXPOOLED],
      [$coinutils_mempool_maxpooled],
      [Default maximum size allocated from pool.])
  fi

  if test $coinutils_mempool_maxpooled -ge 0 ; then
    AC_MSG_NOTICE([CoinUtils mempool enabled for blocks up to $coinutils_mempool_maxpooled bytes.])
    if test $coinutils_mempool_override = yes ; then
      AC_MSG_NOTICE([New/delete in global namespace redirected to CoinUtils mempool.])
    fi
  else
    AC_MSG_NOTICE([CoinUtils mempool will not be used.])
  fi
])
