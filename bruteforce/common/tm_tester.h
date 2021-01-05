#ifndef TM_TESTER_H
#define TM_TESTER_H
#include "tm_base.h"
#include "key_schedule.h"

class tm_tester
{
public:
	tm_tester(TM_base * obj);
	void process_load_fetch(uint8* data_in, uint8* data_out);
	void process_alg_test_case(uint8 * test_case, uint16 * rng_seed, int algorithm);
	uint16 calculate_checksum(uint8* test_case, int world);
	uint16 fetch_checksum_value(uint8* test_case, int world);
	void run_expansion(uint32 key, uint32 data, uint8* result_data);
	void run_full_process(uint32 key, uint32 data, const key_schedule& schedule_entries, uint8* result_data);
	void run_iterations(uint8 * test_case, uint16 * rng_seed, int algorithm, int iterations);
	void run_full_process(uint32 key, uint32 data, const key_schedule& schedule_entries);
	void run_results_process(uint32 key, uint32 data, const key_schedule& schedule_entries, uint32 amount_to_run, uint8* result_data, uint32 result_max_size, uint32* result_size);

private:
	TM_base * UUT;
};
#endif // TM_TESTER_H