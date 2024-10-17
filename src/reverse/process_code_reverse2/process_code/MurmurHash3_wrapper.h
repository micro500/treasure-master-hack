#ifndef MURMURHASH3_WRAPPER_H
#define MURMURHASH3_WRAPPER_H

#include <stdio.h>

#include "data_sizes.h"
#include "MurmurHash3.h"
#include "working_code.h"

const uint32_t hash_seed=2822038001;

struct MurmurHashKey
{
	uint32 value;

	MurmurHashKey(uint32 val) : value(val) {}

	bool operator==(const MurmurHashKey &other) const
	{
		return value == other.value;
	}
};

MurmurHashKey generateHashKey(const working_code& to_hash)
{
	//printf("\nrunning murmur hash on IV: %02X%02X%02X%02X%02X%02X%02X%02X\t\t",to_hash.starting_value[0],to_hash.starting_value[1],to_hash.starting_value[2],to_hash.starting_value[3],to_hash.starting_value[4],to_hash.starting_value[5],to_hash.starting_value[6],to_hash.starting_value[7]);

	uint32 key;
	MurmurHash3_x86_32(to_hash.working_code_data.as_uint8,128,hash_seed,&key);
	//printf("%08X\n",key);

	return MurmurHashKey(key);
}

namespace std {
	/*
	template <>
	struct hash<working_code>
	{
	public:
		size_t operator()(working_code const& to_hash) const 
		{
			printf("\nhashing: %02X%02X%02X%02X%02X%02X%02X%02X\t",to_hash.starting_value[0],to_hash.starting_value[1],to_hash.starting_value[2],to_hash.starting_value[3],to_hash.starting_value[4],to_hash.starting_value[5],to_hash.starting_value[6],to_hash.starting_value[7]);

			uint32 key;
			MurmurHash3_x86_32(to_hash.working_code_data.as_uint8,128,hash_seed,&key);
			printf("%08X\n",key);

			return key;
		}
	};
	*/

	template <>
	struct hash<MurmurHashKey>
	{
	public:
		size_t operator()(MurmurHashKey const& value) const 
		{
			//printf("fake hash: %08X\n",value.value);
			return value.value;
		}
	};
}

#endif // MURMURHASH3_WRAPPER_H