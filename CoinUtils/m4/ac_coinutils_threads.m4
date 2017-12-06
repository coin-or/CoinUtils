
# COINUTILS_THREADS(dflt)
# -------------------------------------------------------------------------
# Control whether or not CoinUtils is thread-aware.
#  dflt ($1) sets the default value; if not given, default is no
# -------------------------------------------------------------------------

AC_DEFUN([AC_COINUTILS_THREADS],
[ 
  AC_ARG_ENABLE([coinutils-threads],
      [ AS_HELP_STRING([--enable-coinutils-threads],
	   [enables compilation of thread aware CoinUtils (mempool so far)])],
      [enable_coinutils_threads=$enableval],
      [enable_coinutils_threads=m4_ifvaln([$1],[$1],[no])])

  if test x"$enable_coinutils_threads" = xyes ; then
    AC_DEFINE([COINUTILS_PTHREADS],[1],
	[Define to 1 if the thread aware version of CoinUtils should be
	 compiled])
    AC_CHECK_LIB([rt],[clock_gettime],
        [COINUTILSLIB_LIBS="-lrt $COINUTILSLIB_LIBS"],
        [AC_MSG_ERROR([--enable-coinutils-threads selected, but -lrt unavailable.])])
    AC_CHECK_LIB([pthread],[pthread_create],
	[COINUTILSLIB_LIBS="-lpthread $COINUTILSLIB_LIBS"],
	[AC_MSG_ERROR([--enable-coinutils-threads selected, but -lpthreads unavailable.])])
    AC_MSG_NOTICE([CoinUtils will be thread-aware.])
  else
    AC_MSG_NOTICE([CoinUtils will not be thread-aware.])
  fi
])

