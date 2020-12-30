#ifndef TM_TESTER_H
#define TM_TESTER_H
#include "tm_base.h"
#include "key_schedule.h"

class tm_tester
{
public:
	tm_tester(TM_base * obj, key_schedule_entry* entries);
	void process_load_fetch(uint8* data_in, uint8* data_out);
	void process_alg_test_case(uint8 * test_case, uint16 * rng_seed, int algorithm);
	uint16 calculate_checksum(uint8* test_case, int world);
	uint16 fetch_checksum_value(uint8* test_case, int world);
	void run_expansion(uint32 key, uint32 data, uint8* result_data);
	void run_full_process(uint32 key, uint32 data, key_schedule_entry* schedule_entries, uint8* result_data);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);
	void run_full_process(uint32 key, uint32 data);

private:
	TM_base * UUT;
	key_schedule_entry * schedule_entries;
};
#endif // TM_TESTER_H