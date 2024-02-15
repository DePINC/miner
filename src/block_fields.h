#ifndef DEPINC_CHIAPOS_BLOCK_FIELDS_H
#define DEPINC_CHIAPOS_BLOCK_FIELDS_H

#include <bls_key.h>
#include <chiapos_types.h>

// #include <serialize.h>
#include <uint256.h>

#include <functional>
#include <vector>

namespace chiapos {

const uint64_t CHIAHEADER_VERSION = 0x108;

class CPosProof {
public:
    uint256 challenge;  // The challenge for PoS

    // The following fields will be used to make PlotID
    Bytes vchPoolPkOrHash;  // Pool public-key (48-byte) or pool contract puzzle hash (32-byte)
    Bytes vchLocalPk;
    Bytes vchFarmerPk;
    uint8_t nPlotType;  // 0 - OGPlot; 1 - PooledPlots

    uint8_t nPlotK;  // The size of the plot
    Bytes vchProof;  // The final proof for the space

    CPosProof() { SetNull(); }

    void SetNull();

    bool IsNull() const;
};

class CVdfProof {
public:
    uint256 challenge;
    Bytes vchY;
    Bytes vchProof;
    uint8_t nWitnessType;
    uint64_t nVdfIters;
    uint64_t nVdfDuration;

    CVdfProof() { SetNull(); }

    void SetNull();

    bool IsNull() const;

    bool Equals(CVdfProof const& rhs) const {
        return challenge == rhs.challenge && vchY == rhs.vchY && vchProof == rhs.vchProof && nWitnessType == rhs.nWitnessType && nVdfIters == rhs.nVdfIters;
    }
};

class CBlockFields {
public:
    uint64_t nVersion{CHIAHEADER_VERSION};
    uint64_t nDifficulty;

    CPosProof posProof;
    CVdfProof vdfProof;

    Bytes vchFarmerSignature;  // A signature by farmer, it should be able to verified by farmer-pubkey

    CBlockFields() { SetNull(); }

    void SetNull();

    bool IsNull() const;

    uint64_t GetTotalIters() const {
        return vdfProof.nVdfIters;
    }

    uint64_t GetTotalDuration() const {
        return vdfProof.nVdfDuration;
    }
};

}  // namespace chiapos

#endif
