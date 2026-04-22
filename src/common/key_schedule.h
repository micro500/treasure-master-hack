#ifndef KEY_SCHEDULE_H
#define KEY_SCHEDULE_H
#include <cstdint>
#include <vector>

class key_schedule
{
public:
	enum map_list_type
	{
		ALL_MAPS,
		SKIP_CAR
	};

	key_schedule(uint32_t key, key_schedule::map_list_type map_list_option);
	key_schedule(uint32_t key, std::vector<uint8_t> map_list);

	void init(uint32_t key, std::vector<uint8_t> map_list);

	typedef struct
	{
		uint8_t rng1;
		uint8_t rng2;
		uint16_t nibble_selector;
	} key_schedule_entry;

	typedef union
	{
		uint8_t as_uint8[4];
		uint16_t as_uint16[2];
		uint32_t as_uint32;
	} key_schedule_data;

	static const int MAX_ENTRIES = 27;
	int entry_count;
	uint16_t seeds[MAX_ENTRIES];
	uint16_t nibble_selectors[MAX_ENTRIES];

	std::vector<key_schedule_entry> entries;

private:
	key_schedule_data schedule_data;

	key_schedule_entry generate_schedule_entry(uint8_t map);
	key_schedule_entry generate_schedule_entry(uint8_t map, uint8_t algorithm);

	void push_entry(key_schedule_entry e);

	void algorithm_0(uint8_t map);
	void algorithm_1(uint8_t map);
	void algorithm_2(uint8_t map);
	void algorithm_3(uint8_t map);
	void algorithm_4(uint8_t map);
	void algorithm_5(uint8_t map);
	void algorithm_6(uint8_t map);
	void algorithm_7(uint8_t map);
};

#endif //KEY_SCHEDULE_H