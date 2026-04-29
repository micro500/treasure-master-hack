#include "tm_cpu_factory.h"

void tm_cpu_factory::register_all_impls() {
    if (!impls_registered) {
        register_impl<tm_8>(ImplType::TM_8, "tm_8", ISA::NONE);
        register_impl<tm_32_8>(ImplType::TM_32_8, "tm_32_8", ISA::NONE);
        register_impl<tm_64_8>(ImplType::TM_64_8, "tm_64_8", ISA::NONE);

        register_impl<tm_ssse3_m128_8>(ImplType::TM_SSSE3_M128_8, "tm_ssse3_m128_8", ISA::SSSE3);
        register_impl<tm_ssse3_m128s_8>(ImplType::TM_SSSE3_M128S_8, "tm_ssse3_m128s_8", ISA::SSSE3);
        register_impl<tm_ssse3_m128_map_8>(ImplType::TM_SSSE3_M128_MAP_8, "tm_ssse3_m128_map_8", ISA::SSSE3);
        register_impl<tm_ssse3_m128s_map_8>(ImplType::TM_SSSE3_M128S_MAP_8, "tm_ssse3_m128s_map_8", ISA::SSSE3);

        register_impl<tm_ssse3_r128_8>(ImplType::TM_SSSE3_R128_8, "tm_ssse3_r128_8", ISA::SSSE3);
        register_impl<tm_ssse3_r128s_8>(ImplType::TM_SSSE3_R128S_8, "tm_ssse3_r128s_8", ISA::SSSE3);
        register_impl<tm_ssse3_r128_map_8>(ImplType::TM_SSSE3_R128_MAP_8, "tm_ssse3_r128_map_8", ISA::SSSE3);
        register_impl<tm_ssse3_r128s_map_8>(ImplType::TM_SSSE3_R128S_MAP_8, "tm_ssse3_r128s_map_8", ISA::SSSE3);

        //register_impl<tm_avx_m256_8>(ImplType::TM_AVX_M256_8, "tm_avx_m256_8", ISA::AVX);

        register_impl<tm_avx_r128_8>(ImplType::TM_AVX_R128_8, "tm_avx_r128_8", ISA::AVX);
        register_impl<tm_avx_r128s_8>(ImplType::TM_AVX_R128S_8, "tm_avx_r128s_8", ISA::AVX);
        register_impl<tm_avx_r128_map_8>(ImplType::TM_AVX_R128_MAP_8, "tm_avx_r128_map_8", ISA::AVX);
        register_impl<tm_avx_r128s_map_8>(ImplType::TM_AVX_R128S_MAP_8, "tm_avx_r128s_map_8", ISA::AVX);

        //register_impl<tm_avx_r256_map_8>(ImplType::TM_AVX_R256_MAP_8, "tm_avx_r256_map_8", ISA::AVX);

        register_impl<tm_avx2_r256_8>(ImplType::TM_AVX2_R256_8, "tm_avx2_r256_8", ISA::AVX2);
        register_impl<tm_avx2_r256s_8>(ImplType::TM_AVX2_R256S_8, "tm_avx2_r256s_8", ISA::AVX2);
        register_impl<tm_avx2_r256_map_8>(ImplType::TM_AVX2_R256_MAP_8, "tm_avx2_r256_map_8", ISA::AVX2);
        register_impl<tm_avx2_r256s_map_8>(ImplType::TM_AVX2_R256S_MAP_8, "tm_avx2_r256s_map_8", ISA::AVX2);

        register_impl<tm_avx2_m256_8>(ImplType::TM_AVX2_M256_8, "tm_avx2_m256_8", ISA::AVX2);
        register_impl<tm_avx2_m256s_8>(ImplType::TM_AVX2_M256S_8, "tm_avx2_m256s_8", ISA::AVX2);
        register_impl<tm_avx2_m256_map_8>(ImplType::TM_AVX2_M256_MAP_8, "tm_avx2_m256_map_8", ISA::AVX2);
        register_impl<tm_avx2_m256s_map_8>(ImplType::TM_AVX2_M256S_MAP_8, "tm_avx2_m256s_map_8", ISA::AVX2);

#ifndef _M_IX86
        register_impl<tm_avx512bw_r512_8>(ImplType::TM_AVX512BW_R512_8, "tm_avx512bw_r512_8", ISA::AVX512BW);
        register_impl<tm_avx512bw_r512s_8>(ImplType::TM_AVX512BW_R512S_8, "tm_avx512bw_r512s_8", ISA::AVX512BW);
        register_impl<tm_avx512bw_r512_map_8>(ImplType::TM_AVX512BW_R512_MAP_8, "tm_avx512bw_r512_map_8", ISA::AVX512BW);
        register_impl<tm_avx512bw_r512s_map_8>(ImplType::TM_AVX512BW_R512S_MAP_8, "tm_avx512bw_r512s_map_8", ISA::AVX512BW);

        register_impl<tm_avx512vl_r128_8>(ImplType::TM_AVX512VL_R128_8, "tm_avx512vl_r128_8", ISA::AVX512VL);
        register_impl<tm_avx512vl_r128s_8>(ImplType::TM_AVX512VL_R128S_8, "tm_avx512vl_r128s_8", ISA::AVX512VL);
        register_impl<tm_avx512vl_r128_map_8>(ImplType::TM_AVX512VL_R128_MAP_8, "tm_avx512vl_r128_map_8", ISA::AVX512VL);
        register_impl<tm_avx512vl_r128s_map_8>(ImplType::TM_AVX512VL_R128S_MAP_8, "tm_avx512vl_r128s_map_8", ISA::AVX512VL);

        register_impl<tm_avx512vl_r256_8>(ImplType::TM_AVX512VL_R256_8, "tm_avx512vl_r256_8", ISA::AVX512VL);
        register_impl<tm_avx512vl_r256s_8>(ImplType::TM_AVX512VL_R256S_8, "tm_avx512vl_r256s_8", ISA::AVX512VL);
        register_impl<tm_avx512vl_r256_map_8>(ImplType::TM_AVX512VL_R256_MAP_8, "tm_avx512vl_r256_map_8", ISA::AVX512VL);
        register_impl<tm_avx512vl_r256s_map_8>(ImplType::TM_AVX512VL_R256S_MAP_8, "tm_avx512vl_r256s_map_8", ISA::AVX512VL);

        register_impl<tm_avx512bwvl_r512_8>(ImplType::TM_AVX512BWVL_R512_8, "tm_avx512bwvl_r512_8", ISA::AVX512BW | ISA::AVX512VL);
        register_impl<tm_avx512bwvl_r512s_8>(ImplType::TM_AVX512BWVL_R512S_8, "tm_avx512bwvl_r512s_8", ISA::AVX512BW | ISA::AVX512VL);
        register_impl<tm_avx512bwvl_r512_map_8>(ImplType::TM_AVX512BWVL_R512_MAP_8, "tm_avx512bwvl_r512_map_8", ISA::AVX512BW | ISA::AVX512VL);
        register_impl<tm_avx512bwvl_r512s_map_8>(ImplType::TM_AVX512BWVL_R512S_MAP_8, "tm_avx512bwvl_r512s_map_8", ISA::AVX512BW | ISA::AVX512VL);
#endif // !_M_IX86

        impls_registered = true;
    }
}

TM_base* tm_cpu_factory::create(ImplType type, RNG* rng, uint32_t key) {
    register_all_impls();
    return registry[type].arg2_factory(rng, key);
}

std::vector<ImplType> tm_cpu_factory::get_impls_for_isa(ISA isa) {
    register_all_impls();
    std::vector<ImplType> result;
    for (auto& [type, info] : registry)
    {
        if ((info.required_isa & isa) == info.required_isa)
        {
            result.push_back(type);
        }
    }
    return result;
}

std::optional<ImplType> tm_cpu_factory::find_by_name(const std::string& name) {
    register_all_impls();
    auto it = name_lookup.find(name);
    if (it == name_lookup.end())
        return std::nullopt;
    return it->second;
}

std::map<ImplType, tm_cpu_factory::ImplInfo> tm_cpu_factory::registry;
std::map<std::string, ImplType> tm_cpu_factory::name_lookup;

bool tm_cpu_factory::impls_registered;