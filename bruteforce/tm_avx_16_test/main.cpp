#include "tm_avx_16_test.h"
#include "tester.h"

int main()
{
	tm_avx_16_test tester;
	run_validity_tests<tm_avx_16_test>(tester);

	run_speed_tests<tm_avx_16_test>(tester, 10000000);
}