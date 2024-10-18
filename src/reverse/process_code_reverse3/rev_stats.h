#pragma once

class rev_stats
{
public:
	rev_stats(int alg0_a, int alg0_m, int alg6_a, int alg6_m) : alg0_available_bits(alg0_a), alg0_mismatch_bits(alg0_m), alg6_available_bits(alg6_a), alg6_mismatch_bits(alg6_m) {};

	int alg0_available_bits;
	int alg0_mismatch_bits;

	int alg6_available_bits;
	int alg6_mismatch_bits;
};