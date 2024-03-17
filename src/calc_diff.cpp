#include "calc_diff.h"

#include <arith_uint256.h>
#include <sha256.h>

#include <cstdint>
#include <limits>

#include "pos.h"
#include "utils.h"

namespace chiapos {

namespace {

arith_uint256 lower_bits(uint256 const& quality_string, int bits) {
    return UintToArith256(quality_string) & (Pow2(bits) - 1);
}

}  // namespace

using QualityBaseType = uint32_t;
constexpr int QUALITY_BASE_BITS = sizeof(QualityBaseType) * 8;

arith_uint256 Pow2(int bits) { return arith_uint256(1) << bits; }

uint256 GenerateMixedQualityString(CPosProof const& posProof) {
    PubKeyOrHash poolPkOrHash =
            MakePubKeyOrHash(static_cast<PlotPubKeyType>(posProof.nPlotType), posProof.vchPoolPkOrHash);
    return chiapos::MakeMixedQualityString(MakeArray<PK_LEN>(posProof.vchLocalPk),
                                           MakeArray<PK_LEN>(posProof.vchFarmerPk), poolPkOrHash, posProof.nPlotK,
                                           posProof.challenge, posProof.vchProof);
}

double CalculateQuality(uint256 const& mixed_quality_string) {
    auto l = lower_bits(mixed_quality_string, QUALITY_BASE_BITS);
    auto h = Pow2(QUALITY_BASE_BITS);
    return static_cast<double>(l.GetLow64()) / static_cast<double>(h.GetLow64());
}

uint64_t CalculateIterationsQuality(uint256 const& mixed_quality_string, uint64_t difficulty, int bits_filter,
                                    int difficulty_constant_factor_bits, uint8_t k, uint64_t base_iters,
                                    double* quality_in_plot, arith_uint256* quality) {
    assert(difficulty > 0);
    auto l = lower_bits(mixed_quality_string, QUALITY_BASE_BITS);
    auto h = Pow2(QUALITY_BASE_BITS);
    auto size = expected_plot_size<arith_uint256>(k);
    auto iters = difficulty * Pow2(difficulty_constant_factor_bits) * l / Pow2(bits_filter) / (size * h) + base_iters;
    if (quality_in_plot) {
        *quality_in_plot = static_cast<double>(l.GetLow64()) / static_cast<double>(h.GetLow64());
    }
    if (quality) {
        *quality = size * h / l;
    }
    if (iters > Pow2(64)) {
        return std::numeric_limits<uint64_t>::max();
    }
    return std::max<uint64_t>(iters.GetLow64(), 1);
}

arith_uint256 CalculateNetworkSpace(uint64_t difficulty, uint64_t iters, int difficulty_constant_factor_bits) {
    arith_uint256 additional_difficulty_constant = Pow2(difficulty_constant_factor_bits);
    return arith_uint256(difficulty) / iters * additional_difficulty_constant * UI_ACTUAL_SPACE_CONSTANT_FACTOR /
           UI_ACTUAL_SPACE_CONSTANT_FACTOR_BASE;
}

}  // namespace chiapos
