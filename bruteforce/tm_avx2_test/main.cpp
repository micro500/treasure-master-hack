#include "tm_avx2_test.h"
#include "tester.h"

int main()
{
	tm_avx2_intrinsics_test tester;
	run_validity_tests<tm_avx2_intrinsics_test>(tester);

	run_speed_tests<tm_avx2_intrinsics_test>(tester, 10000000);
}