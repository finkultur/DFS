/*
 * perfcount.c
 */

#include <stdio.h>
#include <stdint.h>
#include <arch/cycle.h>

#include "perfcount.h"

void
setup_counters(int event1, int event2, int event3, int event4)
{
  __insn_mtspr(SPR_PERF_COUNT_CTL, event1 | (event2 << 16));
  __insn_mtspr(SPR_AUX_PERF_COUNT_CTL, event3 | (event4 << 16));
}

void
read_counters(int* event1, int* event2, int* event3, int* event4)
{
  *event1 = __insn_mfspr(SPR_PERF_COUNT_0);
  *event2 = __insn_mfspr(SPR_PERF_COUNT_1);
  *event3 = __insn_mfspr(SPR_AUX_PERF_COUNT_0);
  *event4 = __insn_mfspr(SPR_AUX_PERF_COUNT_1);
}

void
clear_counters()
{
  __insn_mtspr(SPR_PERF_COUNT_0, 0);
  __insn_mtspr(SPR_PERF_COUNT_1, 0);
  __insn_mtspr(SPR_AUX_PERF_COUNT_0, 0);
  __insn_mtspr(SPR_AUX_PERF_COUNT_1, 0);
}

