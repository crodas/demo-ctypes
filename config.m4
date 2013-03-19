dnl vim:se ts=2 sw=2 et:

PHP_ARG_ENABLE(ctypes, whether to enable ctypes functions,
[  --enable-ctypes         Enable ctypes support])

if test "$PHP_CTYPES" != "no"; then

  PHP_SUBST(CTYPES_SHARED_LIBADD)
  AC_DEFINE(HAVE_ctypes, 1, [ ])

  PHP_NEW_EXTENSION(ctypes, php_ctypes.c resource.c, $ext_shared)
fi

