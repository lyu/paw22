libomp_happy=no

AC_ARG_WITH([libomp], [AS_HELP_STRING([--with-libomp@<:@=DIR@:>@], [Use LLVM/OpenMP offloading])])

AS_IF([test -d "$with_libomp"], [
    libomp_lib_dir="$with_libomp/lib"
    libomp_lib="$libomp_lib_dir/libomptarget.so"
    AS_IF([test -r "$libomp_lib"], [
        LIBOMP_LIBS="-L$with_libomp/lib -Wl,-rpath -Wl,$with_libomp/lib -lomp -lomptarget"
        AC_MSG_NOTICE([libomp: found installation])
        libomp_happy=yes
    ])
])

AS_IF([test "x$libomp_happy" = "xno"], [
    AC_MSG_ERROR([Cannot find required libomp support])
], [
    LDFLAGS="$LIBOMP_LIBS $LDFLAGS"
    AC_DEFINE_UNQUOTED([LIBOMP_LIBS], ["$LIBOMP_LIBS"], [libomp Libraries])
    AC_SUBST(LIBOMP_LIBS)
    AC_DEFINE([HAVE_LIBOMP], [1], [libomp support])
])

AM_CONDITIONAL([HAVE_LIBOMP], [test "x$libomp_happy" != "xno"])
