#ifndef TM_CPU_FACTORY_H
#define TM_CPU_FACTORY_H
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "rng_obj.h"
#include "tm_base.h"
#include "tm_8.h"
#include "tm_32_8.h"
#include "tm_64_8.h"
#include "tm_avx_r128s_8.h"
#include "tm_avx_r128_8.h"
#include "tm_avx_r128_map_8.h"
#include "tm_avx_r128s_map_8.h"
#include "tm_avx_r256_map_8.h"
#include "tm_avx2_r256s_8.h"
#include "tm_avx2_r256_8.h"
#include "tm_avx2_r256_map_8.h"
#include "tm_avx2_r256s_map_8.h"
#include "tm_avx2_m256_8.h"
#include "tm_avx2_m256s_8.h"
#include "tm_avx2_m256_map_8.h"
#include "tm_avx2_m256s_map_8.h"
#ifndef _M_IX86
#include "tm_avx512bw_r512_8.h"
#include "tm_avx512bw_r512s_8.h"
#include "tm_avx512bw_r512_map_8.h"
#include "tm_avx512bw_r512s_map_8.h"
#include "tm_avx512vl_r128_8.h"
#include "tm_avx512vl_r128s_8.h"
#include "tm_avx512vl_r128_map_8.h"
#include "tm_avx512vl_r128s_map_8.h"
#include "tm_avx512vl_r256_8.h"
#include "tm_avx512vl_r256s_8.h"
#include "tm_avx512vl_r256_map_8.h"
#include "tm_avx512vl_r256s_map_8.h"
#include "tm_avx512bwvl_r512_8.h"
#include "tm_avx512bwvl_r512s_8.h"
#include "tm_avx512bwvl_r512_map_8.h"
#include "tm_avx512bwvl_r512s_map_8.h"
#endif // !_M_IX86
#include "tm_avx_m256_8.h"
#include "tm_ssse3_r128_8.h"
#include "tm_ssse3_r128_map_8.h"
#include "tm_ssse3_r128s_map_8.h"
#include "tm_ssse3_m128_8.h"
#include "tm_ssse3_m128s_8.h"
#include "tm_ssse3_m128_map_8.h"
#include "tm_ssse3_m128s_map_8.h"
#include "tm_ssse3_r128s_8.h"

enum class ISA : uint32_t {
    NONE = 0,
    X86_64 = 1 << 0,
    SSSE3 = 1 << 1,
    AVX = 1 << 2,
    AVX2 = 1 << 3,
    AVX512BW = 1 << 4,
    AVX512VL = 1 << 5
};

inline ISA operator|(ISA a, ISA b) {
    return static_cast<ISA>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
    );
}

inline ISA operator&(ISA a, ISA b) {
    return static_cast<ISA>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
    );
}

enum class ImplType {
    TM_8,
    TM_32_8,
    TM_64_8,

    TM_SSSE3_M128_8,
    TM_SSSE3_M128S_8,
    TM_SSSE3_M128_MAP_8,
    TM_SSSE3_M128S_MAP_8,

    TM_SSSE3_R128_8,
    TM_SSSE3_R128S_8,
    TM_SSSE3_R128_MAP_8,
    TM_SSSE3_R128S_MAP_8,
    
    TM_AVX_R128_8,
    TM_AVX_R128S_8,
    TM_AVX_R128_MAP_8,
    TM_AVX_R128S_MAP_8,

    TM_AVX2_R256_8,
    TM_AVX2_R256S_8,
    TM_AVX2_R256_MAP_8,
    TM_AVX2_R256S_MAP_8,

    TM_AVX2_M256_8,
    TM_AVX2_M256S_8,
    TM_AVX2_M256_MAP_8,
    TM_AVX2_M256S_MAP_8,

    TM_AVX512BW_R512_8,
    TM_AVX512BW_R512S_8,
    TM_AVX512BW_R512_MAP_8,
    TM_AVX512BW_R512S_MAP_8,

	TM_AVX512VL_R128_8,
	TM_AVX512VL_R128S_8,
	TM_AVX512VL_R128_MAP_8,
	TM_AVX512VL_R128S_MAP_8,

	TM_AVX512VL_R256_8,
	TM_AVX512VL_R256S_8,
	TM_AVX512VL_R256_MAP_8,
	TM_AVX512VL_R256S_MAP_8,

	TM_AVX512BWVL_R512_8,
	TM_AVX512BWVL_R512S_8,
	TM_AVX512BWVL_R512_MAP_8,
	TM_AVX512BWVL_R512S_MAP_8
};

class tm_cpu_factory
{
    using Factory1 = std::function<TM_base* (RNG*)>;
    using Factory2 = std::function<TM_base* (RNG*, uint32_t)>;
    using Factory3 = std::function<TM_base* (RNG*, uint32_t, const key_schedule&)>;


public:
    template<typename T>
    static void register_impl(ImplType type, const std::string& name, ISA isa) {
        registry[type] = {
            name,
            isa,
            [](RNG* rng) -> TM_base* { return new T(rng); },
            [](RNG* rng, uint32_t key) -> TM_base* { return new T(rng, key); },
            [](RNG* rng, uint32_t key, const key_schedule& s) -> TM_base* { return new T(rng, key, s); }
        };
        name_lookup[name] = type;
    }

    static TM_base* create(ImplType type, RNG* rng, uint32_t key);
    static std::optional<ImplType> find_by_name(const std::string& name);
    static std::vector<ImplType> get_impls_for_isa(ISA isa);
private:

    static void register_all_impls();

    struct ImplInfo {
        std::string name;
        ISA required_isa;
        Factory1 arg1_factory;
        Factory2 arg2_factory;
        Factory3 arg3_factory;
    };

    static std::map<ImplType, ImplInfo> registry;
    static std::map<std::string, ImplType> name_lookup;

	static bool impls_registered;
};

#endif // TM_CPU_FACTORY_H