/*
 * Copyright (c) 2011, Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under, at your option, the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version, or
 * the terms of the simplified BSD license.
 *
 * You should have received a copy of these licenses along this
 * program. If not, see <http://www.gnu.org/licenses/> and
 * <http://www.opensource.org/licenses/bsd-license.html>.
 */

/**
 * @file debug.h
 * @brief debugging macros
 *
 * If NDEBUG is defined at the time this header is included, the
 * macros are ignored.
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#ifndef _DEBUG_H
#define _DEBUG_H

/* NDEBUG already cancels assert() statements in C89 (K&R2, p.254). */
#ifndef NDEBUG

#include <stdio.h>
#include <time.h>

/*
 * DEBUG MESSAGES
 */

/**
 * @brief print debug statements
 *
 * Variadic macros are not ANSI C89, so we define a set of macros with
 * a fixed number of arguments.
 */
#define DBG_PRINTF0(STR) {printf(STR);}
#define DBG_PRINTF1(STR, A1) {printf(STR, A1);}
#define DBG_PRINTF2(STR, A1) {printf(STR, A1, A2);}
#define DBG_PRINTF3(STR, A1) {printf(STR, A1, A2, A3);}
#define DBG_PRINTF4(STR, A1) {printf(STR, A1, A2, A3, A4);}

/*
 * CPU CLOCK TIMER
 */

/**
 * @file debug.h
 *
 * The clock timers are not thread-safe. Your program will not crash and die
 * and burn if you use clock routines in a parallel program, but the
 * numbers may be if clock routines are called from parallel regions.
 *
 * @todo thread-safe clock timing
 */

/** number of clock counters */
#define DBG_CLOCK_NB 16

/** clock counter array, initialized to 0 (K&R2, p.86) */
static clock_t _dbg_clock_counter[DBG_CLOCK_NB];

/**
 * @brief reset a CPU clock counter
 *
 * @param N id of the counter, between 0 and DBG_CLOCK_NB
 */
#define DBG_CLOCK_RESET(N) { _dbg_clock_counter[N] = 0; }

/**
 * @brief toggle (start/stop) a CPU clock counter
 *
 * To measure the CPU clock time taken by an inscruction block, call
 * DBG_CLOCK_TOGGLE() before and after the block. The two successive
 * substractions will increase the counter by the difference of the
 * successive CPU clocks. There is no overflow, _dbg_clock_counter
 * always stays between 0 ans 2x the total execution time.
 *
 * Between the two toggles, the counter values are meaningless.
 * DBG_CLOCK_TOGGLE() must be called an even number of times to make
 * sense.
 *
 * @param N id of the counter, between 0 and DBG_CLOCK_NB
 */
#define DBG_CLOCK_TOGGLE(N) { \
        _dbg_clock_counter[N] = clock() - _dbg_clock_counter[N]; }

/**
 * @brief reset and toggle the CPU clock timer
 *
 * @param N id of the counter, between 0 and DBG_CLOCK_NB
 */
#define DBG_CLOCK_START(N) { \
        DBG_CLOCK_RESET(N); DBG_CLOCK_TOGGLE(N); }

/**
 * @brief CPU clock timer in seconds
 *
 * @param N id of the counter, between 0 and DBG_CLOCK_NB
 */
#define DBG_CLOCK_S(N) ((float) _dbg_clock_counter[N] / CLOCKS_PER_SEC)

/*
 * CPU CYCLES COUNTER
 */

#ifdef _UNDEFINED

static long long _dbg_cycles = 0;

/**
 * @brief reset the cycles counter
 */
#define DBG_CYCLES_RESET() { _dbg_cycles = 0; }

/**
 * @brief increment the cycles counter
 *
 * Start with _dbg_cycles = 0, then run DBG_CYCLES_TOGGLE() before and
 * after the timing blocks. _dbg_cycles always stays >=0, and
 * successive timings will accumulate.
 *
 * DBG_CYCLES_TOGGLE() must be called an even number of times to make sense.
 */
#define DBG_CYCLES_TOGGLE() {                                           \
    unsigned long long counter;                                         \
    asm volatile(".byte 15;.byte 49;shlq $32,%%rdx;orq %%rdx,%%rax"     \
                 : "=a" (counter) ::  "%rdx");                          \
    _dbg_cycles = counter - _dbg_cycles; }

/**
 * @brief reset and toggle the cycles counter
 */
#define DBG_CYCLES_START() { DBG_CYCLES_RESET(); DBG_CYCLES_TOGGLE(); }

#endif

#else

#define DBG_PRINTF0(STR) {}
#define DBG_PRINTF1(STR, A1) {}
#define DBG_PRINTF2(STR, A1) {}
#define DBG_PRINTF3(STR, A1) {}
#define DBG_PRINTF4(STR, A1) {}

#define DBG_CLOCK_NB 0;
#define DBG_CLOCK_RESET(N) {}
#define DBG_CLOCK_TOGGLE(N) {}
#define DBG_CLOCK_START(N) {}
#define DBG_CLOCK_S(N) (-1.)

#endif                          /* !NDEBUG */

#endif                          /* !_DEBUG_H */
