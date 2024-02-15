#ifndef BHD_MINER_PROVER_H
#define BHD_MINER_PROVER_H

#include <chiapos_types.h>
#include <pos.h>
#include <uint256.h>

#include <memory>
#include <vector>

#include <bhd_types.h>

namespace miner {

std::vector<Path> StrListToPathList(std::vector<std::string> const& str_list);

class Prover {
    std::vector<chiapos::CPlotFile> m_plotter_files;

public:
    Prover(std::vector<Path> const& path_list, std::vector<uint8_t> const& allowed_k_vec);

    uint64_t GetTotalSize() const { return m_total_size; }

    uint256 GetGroupHash() const { return m_group_hash; }

    int GetNumOfPlots() const { return m_plotter_files.size(); }

    std::vector<chiapos::QualityStringPack> GetQualityStrings(uint256 const& challenge, int bits_of_filter) const;

    void RevokeByFarmerPk(chiapos::PubKey const& farmer_pk);

    static bool QueryFullProof(Path const& plot_path, uint256 const& challenge, int index, chiapos::Bytes& out,
                               chiapos::PubKey& out_farmer_pk);

    static bool ReadPlotMemo(Path const& plot_file_path, chiapos::PlotMemo& out);

    static chiapos::Bytes CalculateLocalPkBytes(chiapos::Bytes const& local_master_sk);

    static bool VerifyProof(chiapos::Bytes const& plot_id, uint8_t k, uint256 const& challenge,
                            chiapos::Bytes const& proof);

private:
    uint64_t m_total_size{0};
    uint256 m_group_hash;
};

}  // namespace miner

#endif
