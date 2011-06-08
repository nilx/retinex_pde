#!/bin/sh
#
# Test the code compilation and execution.

# simple code execution
_test_run() {
    ./retinex_pde 5 data/noisy.png /tmp/out.png
    test "c77e3675850da101951f230b45d28af5  /tmp/out.png" \
	= "$(md5sum /tmp/out.png)"
}

################################################

_log_init

echo "* default build, clean, rebuild"
_log make distclean
_log make
_log _test_run
_log make
_log make clean
_log make

echo "* standard C compiler, without options"
_log make distclean
_log make CC=cc CFLAGS=
_log _test_run

echo "* tcc C compiler, without options"
_log make distclean
_log make CC=tcc CFLAGS=
_log _test_run

echo "* clang (llvm) C compiler, with and without options"
_log make distclean
_log make CC=clang
_log _test_run
_log make distclean
_log make CC=clang CCFLAGS=
_log _test_run

echo "* standard C++ compiler, with and without options"
_log make distclean
_log make CC=c++
_log _test_run
_log make distclean
_log make CC=c++ CCFLAGS=
_log _test_run

_log make distclean

_log_clean
