#!/bin/sh
#
# Test the code compilation and execution.

# simple code execution
_test_run() {
    TEMPFILE=$(tempfile)
    # 0.019607843137254902 = 5 / 255
    ./retinex_pde 0.019607843137254902 data/noisy.png $TEMPFILE
    test "1c14b80cc282e40d31b707c70973e551  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
    ./retinex_pde 0.019607843137254902 - - < data/noisy.png > $TEMPFILE
    test "1c14b80cc282e40d31b707c70973e551  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)"
}

################################################

_log_init

echo "* default build, clean, rebuild"
_log make -B debug
_log _test_run
_log make -B
_log _test_run
_log make
_log make clean
_log make

echo "* compiler support"
for CC in cc c++ c89 c99 gcc g++ tcc nwcc clang icc pathcc suncc; do
    which $CC || continue
    echo "* $CC compiler"
    _log make distclean
    case $CC in
	"icc")
	    # default icc behaviour is wrong divisions!
	    _log make CC=$CC CFLAGS="-fp-model precise";;
	*)
	    _log make ;;
    esac
    _log _test_run
done

_log make distclean

_log_clean
