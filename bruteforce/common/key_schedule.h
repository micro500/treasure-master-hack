#ifndef KEY_SCHEDULE_H
#define KEY_SCHEDULE_H
#include "data_sizes.h"
#include <vector>

class key_schedule
{
public:
	enum map_list_type
	{
		ALL_MAPS,
		SKIP_CAR
	};

	key_schedule(uint32 key, key_schedule::map_list_type map_list_option);
	key_schedule(uint32 key, std::vector<uint8> map_list);

	void init(uint32 key, std::vector<uint8> map_list);

	typedef struct
	{
		uint8 rng1;
		uint8 rng2;
		uint16 nibble_selector;
	} key_schedule_entry;

	typedef union
	{
		uint8 as_uint8[4];
		uint16 as_uint16[2];
		uint32 as_uint32;
	} key_schedule_data;

	std::vector<key_schedule_entry> entries;

private:
	key_schedule_data schedule_data;

	key_schedule_entry generate_schedule_entry(uint8 map);
	key_schedule_entry generate_schedule_entry(uint8 map, uint8 algorithm);

	void algorithm_0(uint8 map);
	void algorithm_1(uint8 map);
	void algorithm_2(uint8 map);
	void algorithm_3(uint8 map);
	void algorithm_4(uint8 map);
	void algorithm_5(uint8 map);
	void algorithm_6(uint8 map);
	void algorithm_7(uint8 map);
};

#endif //KEY_SCHEDULE_H