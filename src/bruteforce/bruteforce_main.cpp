#include "tm_base.h"
#include "tm_avx_r128s_8.h"
#include "rng_obj.h"
#include "key_schedule.h"

#include <iostream>
#include <array>
#include <unordered_set>

struct ArrayHash {
    std::size_t operator()(const std::array<uint8_t, 128>& arr) const {
        std::size_t seed = 0;
        std::hash<int> hasher;

        for (const auto& elem : arr) {
            seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};

int main()
{
	int key = 0x2CA5B42D;
    //int key = 0xdc53a542; // Bad hash reduction (60% left)
    //int key = 0xac73f512; // Bad hash reduction (60% left)
    //int key = 0x36da9f03e;
    //int key = 0x00000000;
	//int data = 0x0735B1D2;

	key_schedule schedule_data(key, key_schedule::ALL_MAPS);

	RNG rng;

	tm_avx_r128s_8 tm(&rng);


    std::unordered_set<std::array<uint8_t, 128>, ArrayHash> mySet;
    mySet.reserve(1000000);


    for (uint32_t i = 0; i < 0xF0000000; i++)
    {
        tm.run_first_map(key, i, schedule_data);

        std::array<uint8_t, 128> key1;
        std::copy(std::begin(tm.working_code_data), std::end(tm.working_code_data), key1.begin());

        auto x = mySet.insert(key1);

        /*if (!x.second) {
            std::cout << "key already existed " << i << std::endl;
        }*/
    }

    std::cout << mySet.size() << std::endl;
}
