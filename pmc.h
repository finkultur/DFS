/* pmc.h
 *
 * Handling and reading of the PMC registers on the TILEPRO64. */

#define SPR_PERF_COUNT_CTL  0x4207
#define SPR_AUX_PERF_COUNT_CTL  0x6007

#define SPR_PERF_COUNT_0 0x4205
#define SPR_PERF_COUNT_1 0x4206
#define SPR_AUX_PERF_COUNT_0 0x6005
#define SPR_AUX_PERF_COUNT_1 0x6006

#define BUNDLES_RETIRED 0x6
#define DATA_CACHE_STALL 0x10
#define INST_CACHE_STALL 0x11
#define CACHE_BUSY_STALL 0x18

#define LOCAL_WR_MISS 0x35
#define LOCAL_WR_CNT 0x29
#define LOCAL_DRD_MISS 0x34
#define LOCAL_DRD_CNT 0x28

/* Setup PMCs to count the specified events. */
void setup_counters(int event1, int event2, int event3, int event4);

/* Reads PMCs and stores their values in the specified locations. */
void read_counters(unsigned int* event1, unsigned int* event2,
		unsigned int* event3, unsigned int* event4);

/* Clears the PMCs. */
void clear_counters(void);
