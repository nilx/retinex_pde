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
 * @brief debugging and profiling macros
 *
 * If NDEBUG is defined at the time this header is included, the
 * macros are ignored. We chose NDEBUG because this macro already
 * cancels assert() statements in C89 (K&R2, p.254).
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <time.h>

/*
 * DEBUG MESSAGES
 */

#ifndef NDEBUG

/**
 * @brief printf()-like debug statements
 *
 * Variadic macros are not ANSI C89, so we define a set of macros with
 * a fixed number of arguments.
 *
 * Use like printf():
 * DBG_PRINTF("entering function foo()\n");
 * DBG_PRINTF2("i=%i, j=%i\n", i, j);
 */
#define DBG_PRINTF0(STR) \
    { fprintf(stderr, STR); }
#define DBG_PRINTF1(STR, A1) \
    { fprintf(stderr, STR, A1); }
#define DBG_PRINTF2(STR, A1, A2) \
    { fprintf(stderr, STR, A1, A2); }
#define DBG_PRINTF3(STR, A1, A2, A3) \
    { fprintf(stderr, STR, A1, A2, A3); }
#define DBG_PRINTF4(STR, A1, A2, A3, A4) \
    { fprintf(stderr, STR, A1, A2, A3, A4); }

#else

#define DBG_PRINTF0(STR) {}
#define DBG_PRINTF1(STR, A1) {}
#define DBG_PRINTF2(STR, A1, A2) {}
#define DBG_PRINTF3(STR, A1, A2, A3) {}
#define DBG_PRINTF4(STR, A1, A2, A3, A4) {}

#endif                          /* !NDEBUG */

/*
 * CPU CLOCK TIMER
 */

/**
 * @file debug.h
 *
 * Some clock counters are available at the compilation unit (source
 * file) level. Each counter can be toggled on/off with
 * DBG_CLOCK_TOGGLE(N), with N between 0 and DBG_CLOCK_NB - 1.
 * Successive on/off toggle of the same counter, even from different
 * functions or different calls to the same function, will add clock
 * time until the counter is reset with DBG_CLOCK_RESET(N). The
 * counter time can be read in seconds (float) with DBG_CLOCK_S(N) and
 * its raw value (long) with DBG_CLOCK(N).
 *
 * Between the two toggles, the counter values are meaningless.
 * DBG_CLOCK_TOGGLE() must be called an even number of times to make
 * sense.
 *
 * These macros use the C89 libc clock() call, they measure the CPU
 * time used. Note that this is the total CPU time, on all CPUs, not
 * the "wall clock" time: a process running on 8 CPUs for 2 seconds
 * uses 16s of CPU time. The CPU time is not affected directly by
 * other programs running on the machine, but there may be some
 * side-effect because of the ressource conflicts like CPU cache reload.
 * These macros are suitable when the measured time is around a second
 * or more.
 *
 * The clock counters are not thread-safe. Your program will not crash
 * and die and burn if you use clock macros in a parallel program,
 * but the numbers may be wrong be if clock macros are called in parallel.
 *
 * Usage:
 *   DBG_CLOCK_RESET(N);
 *   for(i = 0; i < large_number; i++) {
 *     DBG_CLOCK_TOGGLE(N);
 *     some_operations
 *     DBG_CLOCK_TOGGLE(N);
 *     other_operations
 *   }
 *   DBG_PRINTF1("CPU time spent in some_ops: %0.3fs\n", DBG_CLOCK_S(N));
 *
 * @todo OpenMP-aware clock timing
 */

#ifndef NDEBUG

/** number of clock counters */
#define DBG_CLOCK_NB 16

/** clock counter array, initialized to 0 (K&R2, p.86) */
static clock_t _dbg_clock_counter[DBG_CLOCK_NB];

/**
 * @brief reset a CPU clock counter
 */
#define DBG_CLOCK_RESET(N) { _dbg_clock_counter[N] = 0; }

/**
 * @brief toggle (start/stop) a CPU clock counter
 *
 * To measure the CPU clock time used by an instruction block, call
 * DBG_CLOCK_TOGGLE() before and after the block.
 *
 * The two successive substractions will increase the counter by the
 * difference of the successive CPU clocks. There is no overflow,
 * _dbg_clock_counter always stays between 0 and 2x the total
 * execution time.
 */
#define DBG_CLOCK_TOGGLE(N) { \
        _dbg_clock_counter[N] = clock() - _dbg_clock_counter[N]; }

/**
 * @brief reset and toggle the CPU clock counter
 */
#define DBG_CLOCK_START(N) { \
        DBG_CLOCK_RESET(N); DBG_CLOCK_TOGGLE(N); }

/**
 * @brief CPU clock counter
 */
#define DBG_CLOCK(N) ((long) _dbg_clock_counter[N])

/**
 * @brief CPU clock time in seconds
 */
#define DBG_CLOCK_S(N) ((float) _dbg_clock_counter[N] / CLOCKS_PER_SEC)

#else

#define DBG_CLOCK_NB 0;
#define DBG_CLOCK_RESET(N) {}
#define DBG_CLOCK_TOGGLE(N) {}
#define DBG_CLOCK_START(N) {}
#define DBG_CLOCK(N) (-1)
#define DBG_CLOCK_S(N) (-1.)

#endif                          /* !NDEBUG */

/*
 * CPU CYCLES COUNTER
 */

/**
 * @file debug.h
 *
 * Some CPU cycle counters are available and follow the same model as
 * the clock counters. Cycle counters are toggled on/off with
 * DBG_CYCLE_TOGGLE(N), with N between 0 and DBG_CYCLE_NB - 1.
 * Counters are reset with DBG_CYCLE_RESET(N) and can be read with
 * DBG_CYCLE(N).
 *
 * These macros use the work of Daniel J. Bernstein:
 *   http://ebats.cr.yp.to/cpucycles.html
 *   http://ebats.cr.yp.to/cpucycles-20060326.tar.gz
 *
 * These macros are suitable to measure the CPU cost of a few
 * instructions, and should not be used when multitasking interrupts
 * the measure.
 *
 * Usage:
 *   DBG_CYCLE_RESET(N);
 *   for(i = 0; i < large_number; i++) {
 *     DBG_CYCLE_STARTTOGGLE(N);
 *     some_operations
 *     DBG_CLOCK_TOGGLE(N);
 *     other_operations
 *   }
 *   DBG_PRINTF1("Total CPU cycles spent in some_ops: %lld\n", DBG_CYCLE(N));
 *
 * For good results, repeat the measures many times, take the median,
 * and remove the cost of the measure itself (median time between two
 * adjacent measures).
 *
 * Usage:
 *   long long cycles[large_number];
 *   for(i = 0; i < large_number; i++) {
 *     DBG_CYCLE_START(N);
 *     cycles[i] = DBG_CYCLE(N);
 *   }
 *   qsort(cycles, large_number, sizeof(long long), &cmp);
 *   long long tmp = cycles[large_number / 2];
 *
 *   for(i = 0; i < large_number; i++) {
 *     DBG_CYCLE_START(N);
 *     some_operations
 *     cycles[i] = DBG_CYCLE(N);
 *     other_operations
 *   }
 *   for(i = 0; i < large_number; i++)
 *     cycles[i] -= tmp;
 *   qsort(cycles, large_number, sizeof(long long), &cmp);
 *   DBG_PRINTF1("Median CPU cycles spent in some_ops: %lld\n",
 *               cycles[large_number / 2]);
 *
 * You can insert #ifdefs to use the same code for debug and prod.
 *
 * Theses macros are not for parallel programs. They are only
 * available on x86 and amd64 hardware.
 *
 * When compiled on an ANSI C90 compiler, these cycle counter type is
 * "long" instead of "long long".
 *
 * @todo more architectures
 */

#ifndef NDEBUG

#if (defined(__STDC__) && defined(__STDC_VERSION__) \
     && (__STDC_VERSION__ >= 199409L))
#define _LL long long
#else
#define _LL long
#endif

#if (defined(__amd64__) || defined(__amd64) || defined(_M_X64))
/* from http://predef.sourceforge.net/prearch.html#sec3 */

/** CPU cycles counter for x86 */
static _LL _dbg_cpucycles(void)
{
    _LL result;
    __asm__ volatile (".byte 15;.byte 49":"=A" (result));
    return result;
}

#elif (defined(__i386__) || defined(__i386) || defined(_M_IX86) \
       || defined(__X86__) || defined(_X86_) || defined(__I86__))
/* from http://predef.sourceforge.net/prearch.html#sec6 */

/** CPU cycles counter for amd64 */
static _LL _dbg_cpucycles(void)
{
    unsigned _LL result;
    __asm__ volatile (".byte 15;.byte 49;shlq $32,%%rdx;orq %%rdx,%%rax":"=a"
                      (result)::"%rdx");
    return result;
}

#else

/** dummy CPU cycles counter */
static _LL _dbg_cpucycles(void)
{
    return 0;
}

#endif                          /* architecture detection */

/** number of cycle counters */
#define DBG_CYCLE_NB 16

/** cycle counter array, initialized to 0 */
static _LL _dbg_cycle_counter[DBG_CYCLE_NB];

/**
 * @brief reset a CPU cycle counter
 */
#define DBG_CYCLE_RESET(N) { _dbg_cycle_counter[N] = 0; }

/**
 * @brief toggle (start/stop) a CPU cycle counter
 *
 * To measure the CPU cycles used by an instruction block, call
 * DBG_CYCLE_TOGGLE() before and after the block.
 */
#define DBG_CYCLE_TOGGLE(N) { \
        _dbg_cycle_counter[N] = _dbg_cpucycles() - _dbg_cycle_counter[N]; }

/**
 * @brief reset and toggle the CPU cycles counter
 */
#define DBG_CYCLE_START(N) { \
        DBG_CYCLE_RESET(N); DBG_CYCLE_TOGGLE(N); }

/**
 * @brief CPU cycle counter
 */
#define DBG_CYCLE(N) (_dbg_cycle_counter[N])

#undef _LL

#else

#define DBG_CYCLE_NB 0;
#define DBG_CYCLE_RESET(N) {}
#define DBG_CYCLE_TOGGLE(N) {}
#define DBG_CYCLE_START(N) {}
#define DBG_CYCLE(N) (-1)

#endif                          /* !NDEBUG */

#endif                          /* !_DEBUG_H */
