#ifndef _print_debug_h_
#define _print_debug_

#include "WString.h"
#include "hal.h"
#include "stdio.h"

template <typename... T>
void PRINT_DEBUG(int X, const __FlashStringHelper *str, T const... div) {
  if (debugLevel >= X) {
#ifdef ARDUINO_ARCH_ESP32
    // on ESP the MACRO PRIu32 cause a problem with syntax below
    printf("%u ", hal_ticks().tick());
#else
    printf_P(PSTR("%" PRIu32 " "), hal_ticks().tick());
#endif
    PGM_P p = reinterpret_cast<PGM_P>(str);
    printf_P(p, div...);
    // printf_P(PSTR("\n"));
    printf("\n");
  }
}

constexpr bool IS_DEBUG_ENABLE(int x) { return debugLevel >= x; }

void hal_printf_init();

#endif