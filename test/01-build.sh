#!/bin/sh
#
# Test the code compilation and execution.

# simple code execution
_test_run() {
    TEMPFILE=$(tempfile)
    # 0.019607843137254902 = 5 / 255
    ./retinex_pde 0.019607843137254902 data/noisy.png $TEMPFILE
    test "1c14b80cc282e40d31b707c70973e551  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)" \
	-o "b19dfafff39f3337b579cdd5df431c9a  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)" # Win32 fftw3 has different rounding
    ./retinex_pde 0.019607843137254902 - - < data/noisy.png > $TEMPFILE
    test "1c14b80cc282e40d31b707c70973e551  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)" \
	-o "b19dfafff39f3337b579cdd5df431c9a  $TEMPFILE" \
	= "$(md5sum $TEMPFILE)" # Win32 fftw3 has different rounding
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
#for CC in cc c++ c89 c99 gcc g++ tcc nwcc clang icc pathcc suncc \
for CC in cc c++ c89 c99 gcc g++ tcc clang icc suncc \
    i586-mingw32msvc-cc; do
    which $CC || continue
    echo "* $CC compiler"
    _log make distclean
    case $CC in
	"i586-mingw32msvc-cc")
	    test -d ./win32/lib || break
	    # default mingw behaviour has extra precision
	    _log make CC=$CC CFLAGS="-O2 -ffloat-store"\
		CPPFLAGS="-DNDEBUG -I. -I./win32/include" \
		LDFLAGS="-L./win32/lib" \
		LDLIBS="-lpng -lfftw3f-3 -lm"
	    ln -f -s ./win32/bin/libpng3.dll ./win32/bin/zlib1.dll \
		./win32/bin/libfftw3f-3.dll ./
	    ;;
	"icc")
	    # default icc behaviour is wrong divisions!
	    _log make CC=$CC CFLAGS="-fp-model precise"
	    ;;
	*)
	    _log make CC=$CC
	    ;;
    esac
    _log _test_run
    rm -f ./*.dll
done

_log make distclean

_log_clean
