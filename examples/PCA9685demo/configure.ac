AC_PREREQ([2.69])
AC_INIT([PCA9685demo], [0.2])

AC_CONFIG_SRCDIR([PCA9685demo.c])

# Switch to a C compiler, and check if it works.
AC_LANG([C])
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday select])

# Check for the required libraries
AC_CHECK_HEADERS([ncurses.h], [], [AC_ERROR([A working ncurses lib is required])])
AC_SEARCH_LIBS([initscr], [ncurses], [], [AC_ERROR([A working ncurses lib is required])])

AC_CHECK_HEADERS([PCA9685.h], [], [AC_ERROR([A working PCA9685 lib is required])])
AC_SEARCH_LIBS([PCA9685_openI2C], [PCA9685], [], [AC_ERROR([A working PCA9685 lib is required])])

# Process Makefile.in to create Makefile
AC_OUTPUT([Makefile])
