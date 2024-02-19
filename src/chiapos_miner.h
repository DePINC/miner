#ifndef BHD_MINER_MINER_HPP
#define BHD_MINER_MINER_HPP

#include <bls_key.h>
#include <chiapos_types.h>
#include <timelord_client.h>

#include <functional>
#include <mutex>
#include <string>
#include <set>
#include <optional>

#include <tinyformat.h>

#include "prover.h"
#include "rpc_client.h"

namespace miner {
namespace pos {
chiapos::optional<RPCClient::PosProof> QueryBestPosProof(Prover& prover, uint256 const& challenge, uint64_t difficulty,
                                                         int difficulty_constant_factor_bits, int filter_bits,
                                                         int base_iters, chiapos::PubKey& out_farmer_pk,
                                                         std::string* out_plot_path = nullptr);
}

using TimelordClientPtr = std::shared_ptr<TimelordClient>;

struct EndpointDesc {
    std::string hostname;
    uint16_t port;
    std::string ToString() const { return tinyformat::format("%s:%d", hostname, port); }
    bool operator<(EndpointDesc const& rhs) const { return ToString() < rhs.ToString(); }
};

struct ClientDesc {
    bool reconnecting;
    TimelordClientPtr pclient;
};

/// Miner is a state machine
class Miner {
public:
    Miner(RPCClient& client, Prover& prover, std::map<chiapos::PubKey, chiapos::SecreKey> secre_keys,
          std::string reward_dest, int difficulty_constant_factor_bits, bool no_cuda, int max_compression_level, int timeout_seconds);

    ~Miner();

    void StartTimelord(std::vector<std::string> const& endpoints, uint16_t default_port);

    int Run();

private:
    enum class State { RequireChallenge, FindPoS, WaitVDF, ProcessVDF, SubmitProofs };

    enum class BreakReason { Error, Timeout, ChallengeIsChanged, VDFIsAcquired };

    TimelordClientPtr PrepareTimelordClient(std::string const& hostname, unsigned short port);

    /// A thread proc to check the challenge or the VDF from P2P network
    BreakReason CheckAndBreak(std::atomic_bool& running, int timeout_seconds, uint256 const& initial_challenge,
                              uint256 const& current_challenge, uint64_t iters_limits, uint256 const& group_hash,
                              uint64_t total_size, chiapos::optional<RPCClient::VdfProof>& out_vdf);

    static std::string ToString(State state);

    void TimelordProc();

    chiapos::optional<ProofDetail> QueryProofFromTimelord(uint256 const& challenge, uint64_t iters) const;

    void SaveProof(uint256 const& challenge, ProofDetail const& detail);

private:
    // utilities
    RPCClient& m_client;
    Prover& m_prover;
    std::map<chiapos::PubKey, chiapos::SecreKey> m_secre_keys;
    std::string m_reward_dest;
    int m_difficulty_constant_factor_bits;
    // State
    std::atomic<State> m_state{State::RequireChallenge};
    // thread and timelord
    asio::io_context m_ioc;
    std::unique_ptr<std::thread> m_pthread_timelord;
    std::map<EndpointDesc, ClientDesc> m_timelords;
    mutable std::mutex m_mtx_proofs;
    std::map<uint256, std::vector<ProofDetail>> m_proofs;
    std::set<uint256> m_submit_history;
    std::atomic_bool m_shutting_down{false};
    // temporary save the current challenge/iters
    uint256 m_current_challenge;
    uint64_t m_current_iters;
};

}  // namespace miner

#endif
