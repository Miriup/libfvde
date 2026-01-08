dnl Checks for keyutils required headers and functions
dnl
dnl Version: 20260108

dnl Function to detect if keyutils is available
AC_DEFUN([AX_KEYUTILS_CHECK_LIB],
  [AS_IF(
    [test "x$ac_cv_enable_shared_libs" = xno || test "x$ac_cv_with_keyutils" = xno],
    [ac_cv_keyutils=no],
    [AC_CHECK_HEADERS([keyutils.h])

    AS_IF(
      [test "x$ac_cv_header_keyutils_h" = xno],
      [ac_cv_keyutils=no],
      [AC_CHECK_LIB(
        keyutils,
        add_key,
        [ac_cv_keyutils=yes],
        [ac_cv_keyutils=no])

      AS_IF(
        [test "x$ac_cv_keyutils" = xyes],
        [ac_cv_keyutils_LIBADD="-lkeyutils"])
      ])
    ])

  AS_IF(
    [test "x$ac_cv_keyutils" = xyes],
    [AC_DEFINE(
      [HAVE_KEYUTILS],
      [1],
      [Define to 1 if you have the 'keyutils' library (-lkeyutils).])
    AC_DEFINE(
      [HAVE_KEYUTILS_H],
      [1],
      [Define to 1 if you have the <keyutils.h> header file.])
    ])

  AM_CONDITIONAL(
    [HAVE_KEYUTILS],
    [test "x$ac_cv_keyutils" = xyes])
  ])

dnl Function to detect how to enable keyutils
AC_DEFUN([AX_KEYUTILS_CHECK_ENABLE],
  [AX_COMMON_ARG_WITH(
    [keyutils],
    [keyutils],
    [search for keyutils in includedir and libdir or in the specified DIR, or no if not to use keyutils],
    [auto-detect],
    [DIR])

  dnl Check for a shared library version
  AX_KEYUTILS_CHECK_LIB

  AS_IF(
    [test "x$ac_cv_keyutils_LIBADD" != "x"],
    [AC_SUBST(
      [LIBKEYUTILS_LIBADD],
      [$ac_cv_keyutils_LIBADD])
    ])
  ])
