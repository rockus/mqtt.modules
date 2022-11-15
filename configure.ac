
# https://robots.thoughtbot.com/the-magic-behind-configure-make-make-install
# https://autotools.io/autoconf/arguments.html
# https://www.gnu.org/software/automake/manual/html_node/Subdirectories-with-AM_005fCONDITIONAL.html#Subdirectories-with-AM_005fCONDITIONAL
# https://github.com/securestate/kraken/blob/master/configure.ac

#    aclocal && automake --add-missing && autoconf && ./configure && make

AC_INIT(mqtt.modules, 1.0, rockus@rockus.at)
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_CONFIG_FILES([Makefile raspi_pulsecount/Makefile bme280/Makefile])

dnl raspi_pulsecount code enabled by default
AC_ARG_ENABLE([raspi_pulsecount], AS_HELP_STRING(--disable-raspi_pulsecount, [disable raspi_pulsecount module]))
AM_CONDITIONAL([raspi_pulsecount_cond], [test "x$enable_raspi_pulsecount" != xno])

dnl bme280 code enabled by default
AC_ARG_ENABLE([bme280], AS_HELP_STRING(--disable-bme280, [disable bme280 module]))
AM_CONDITIONAL([bme280_cond], [test "x$enable_bme280" != xno])



dnl all modules need libconfig, libmosquitto, real-time kernel enhancements and threads
AC_CHECK_LIB([config], [config_init], [], [AC_MSG_ERROR([Library config9 not found.])])
AC_CHECK_LIB([mosquitto], [mosquitto_lib_init], [], [AC_MSG_ERROR([Library mosquitto not found.])])
LIBS="$LIBS -lrt -lpthread"


dnl WiringPi required by bme280, raspi_pulsecount


need_libwiringpi="no"

if test "x$enable_raspi_pulsecount" != "xno"; then
  need_libwiringpi="yes"
fi
if test "x$enable_bme280" != "xno"; then
  need_libwiringpi="yes"
fi


if test "x$need_libwiringpi" != "xno"; then
  AC_CHECK_LIB([wiringPi], [wiringPiSetup], [], [AC_MSG_ERROR([Library wiringPi not found.])])
fi

AC_OUTPUT
