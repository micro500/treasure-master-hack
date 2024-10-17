#pragma once
#include <cstdint>

class minterm
{
public:
	minterm(uint32_t value, uint32_t mask);
	virtual ~minterm();

	uint32_t minterm_val;
	uint32_t minterm_mask;
};