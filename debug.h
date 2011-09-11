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
 * The timer is very simple:
 * - only one counter
 * - not thread-safe
 *
 * Nested timing is not possible within a compilation unit (file), but
 * possible when functions are called over different compilation
 * units. Your program will not crash and die and burn if you use
 * nested timings or timer routines in a parallel program, but the
 * numbers may be if clock functions are called from regions.
 *
 * @todo multiple counters
 * @todo thread-safe timing
 * @todo cycle timing (cf. http://www.ecrypt.eu.org/ebats/cpucycles.html)
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 */

#ifndef _DEBUG_H
#define _DEBUG_H

/* NDEBUG already cancels assert() statements in C89 (K&R2, p.254). */
#ifndef NDEBUG

#include <stdio.h>
#include <time.h>

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

static clock_t _dbg_timer = 0;

/**
 * @brief reset the CPU clock timer
 */
#define DBG_CLOCK_RESET() {_dbg_timer = 0;}

/**
 * @brief increment the CPU clock timer
 *
 * Start with timer = 0, then run DBG_CLOCK_TOGGLE() before and after the
 * timing blocks. _dbg_timer always stays >=0, and successive timings will
 * accumulate.
 *
 * CLOCK() must be called an even number of times.
 */
#define DBG_CLOCK_TOGGLE() {_dbg_timer = clock() - _dbg_timer;}

/**
 * @brief reset and toggle the CPU clock timer
 */
#define DBG_CLOCK_START() {DBG_CLOCK_RESET(); DBG_CLOCK_TOGGLE();}

/**
 * @brief CPU clock timer in seconds
 */
#define DBG_CLOCK_S() ((float) _dbg_timer / CLOCKS_PER_SEC)

#else

#define DBG_PRINTF0(STR) {}
#define DBG_PRINTF1(STR, A1) {}
#define DBG_PRINTF2(STR, A1) {}
#define DBG_PRINTF3(STR, A1) {}
#define DBG_PRINTF4(STR, A1) {}

#define DBG_CLOCK_RESET() {}
#define DBG_CLOCK_TOGGLE() {}
#define DBG_CLOCK_START() {}
#define DBG_CLOCK_S() (-1.)

#endif                          /* !NDEBUG */

#endif                          /* !_DEBUG_H */
