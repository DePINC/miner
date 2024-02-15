#ifndef BHD_MINER_RPC_CLIENT_HPP
#define BHD_MINER_RPC_CLIENT_HPP

#include <bls_key.h>
#include <chiapos_types.h>
#include <pos.h>
#include <utils.h>
#include <vdf.h>
#include <curl/curl.h>
#include <plog/Log.h>
// #include <script/standard.h>
#include <univalue.h>
#include <vdf_computer.h>
#include <uint256.h>

#include <cstdint>
#include <string>

#include "http_client.h"

namespace miner {

enum class DepositTerm { NoTerm = 0, Term1, Term2, Term3 };

std::string DepositTermToString(DepositTerm term);

DepositTerm DepositTermFromString(std::string const& str);

class Error : public std::runtime_error {
public:
    explicit Error(char const* msg) : std::runtime_error(msg) {}
};

class NetError : public Error {
public:
    explicit NetError(char const* msg) : Error(msg) {}
};

class RPCError : public Error {
public:
    RPCError(int code, std::string msg) : Error(msg.c_str()), m_code(code), m_msg(std::move(msg)) {}

    int GetCode() const { return m_code; }

private:
    int m_code;
    std::string m_msg;
};

class RPCClient {
public:
    struct Result {
        UniValue result;
        std::string error;
        int id;
    };

    struct VdfProof {
        uint256 challenge;
        chiapos::VdfForm y;
        chiapos::Bytes proof;
        uint64_t iters;
        uint8_t witness_type;
        uint64_t duration;
    };

    struct Challenge {
        uint256 challenge;
        uint64_t difficulty;
        uint256 prev_block_hash;
        int prev_block_height;
        uint64_t prev_vdf_iters;
        uint64_t prev_vdf_duration;
        int target_height;
        uint64_t target_duration;
        int filter_bits;
        int base_iters;
        std::vector<VdfProof> vdf_proofs;
    };

    struct PosProof {
        uint256 mixed_quality_string;
        uint64_t iters;
        uint256 challenge;
        uint8_t k;
        chiapos::PlotId plot_id;
        chiapos::PubKeyOrHash pool_pk_or_hash;
        chiapos::PubKey local_pk;
        chiapos::Bytes proof;
    };

    struct ProofPack {
        uint256 prev_block_hash;
        int prev_block_height;
        PosProof pos;
        VdfProof vdf;
        chiapos::SecreKey farmer_sk;
        std::string reward_dest;
    };

    struct BindRecord {
        chiapos::Bytes tx_id;
        std::string address;
        std::string farmer_pk;
        chiapos::Bytes block_hash;
        int block_height;
        bool active;
        bool valid;
    };

    struct PledgeParams {
        uint64_t netCapacityTB;
        int calculatedOnHeight;
        uint64_t supplied;
    };

    struct MiningRequirement {
        std::string address;
        CAmount req;
        int mined_count;
        int total_count;
        CAmount burned;
        CAmount supplied;
        CAmount accumulate;
        int height;
        chiapos::Bytes farmer_pk;
    };

    struct PledgeRecord {
        chiapos::Bytes tx_id;
        int height{0};
        double amount;
        std::string from;
        std::string to;
        DepositTerm term;
        bool retarget{false};
        int point_height{0};
        bool valid;
        bool revoked;
    };

    RPCClient(bool no_proxy, std::string url, std::string const& cookie_path_str = "");

    RPCClient(bool no_proxy, std::string url, std::string user, std::string passwd);

    void SetWallet(std::string const& wallet_name);

    void LoadCookie();

    std::string const& GetCookiePath() const;

    bool CheckChiapos();

    Challenge QueryChallenge();

    PledgeParams QueryNetspace();

    void SubmitProof(ProofPack const& proof_pack);

    chiapos::Bytes BindPlotter(std::string const& address, chiapos::SecreKey const& farmerSk, int spend_height);

    std::vector<BindRecord> ListBindTxs(std::string const& address, int count = 10, int skip = 0,
                                        bool include_watchonly = false, bool include_invalid = false);

    chiapos::Bytes Deposit(std::string const& address, int amount, DepositTerm term);

    chiapos::Bytes Withdraw(chiapos::Bytes const& tx_id);

    bool GenerateBurstBlocks(int count);

    chiapos::Bytes RetargetPledge(chiapos::Bytes const& tx_id, std::string const& address);

    MiningRequirement QueryMiningRequirement(std::string const& address, chiapos::PubKey const& farmer_pk);

    bool SubmitVdfRequest(uint256 const& challenge, uint64_t iters);

private:
    void BuildRPCJson(UniValue& params, std::string const& val);

    void BuildRPCJson(UniValue& params, chiapos::Bytes const& val);

    void BuildRPCJson(UniValue& params, uint256 const& val);

    void BuildRPCJson(UniValue& params, bool val);

    void BuildRPCJson(UniValue& params, PosProof const& proof);

    void BuildRPCJson(UniValue& params, VdfProof const& proof);

    template <size_t N>
    void BuildRPCJson(UniValue& params, std::array<uint8_t, N> const& val) {
        BuildRPCJson(params, chiapos::BytesToHex(chiapos::MakeBytes(val)));
    }

    template <typename T>
    void BuildRPCJson(UniValue& params, std::vector<T> const& val) {
        UniValue res(UniValue::VARR);
        for (auto const& v : val) {
            BuildRPCJson(res, v);
        }
        params.push_back(res);
    }

    template <typename T>
    void BuildRPCJson(UniValue& params, T&& val) {
        params.push_back(std::forward<T>(val));
    }

    void BuildRPCJsonWithParams(UniValue& out_params);

    template <typename V, typename... T>
    void BuildRPCJsonWithParams(UniValue& outParams, V&& val, T&&... vals) {
        BuildRPCJson(outParams, std::forward<V>(val));
        BuildRPCJsonWithParams(outParams, std::forward<T>(vals)...);
    }

    template <typename... T>
    Result SendMethod(bool no_proxy, std::string const& method_name, T&&... vals) {
        UniValue root(UniValue::VOBJ);
        root.pushKV("jsonrpc", "2.0");
        root.pushKV("method", method_name);
        UniValue params(UniValue::VARR);
        BuildRPCJsonWithParams(params, std::forward<T>(vals)...);
        root.pushKV("params", params);
        // Invoke curl
        std::string url_with_wallet;
        if (m_wallet_name.empty()) {
            url_with_wallet = m_url;
        } else {
            url_with_wallet = m_url + "/wallet/" + m_wallet_name;
        }
        HTTPClient client(url_with_wallet, m_user, m_passwd, no_proxy);
        std::string send_str = root.write();
        PLOG_DEBUG << "sending: `" << send_str << "`";
        bool succ;
        int code;
        std::string err_str;
        std::tie(succ, code, err_str) = client.Send(send_str);
        if (!succ) {
            std::stringstream ss;
            ss << "RPC command error `" << method_name << "`: " << err_str;
            throw NetError(ss.str().c_str());
        }
        // Analyze the result
        chiapos::Bytes received_data = client.GetReceivedData();
        if (received_data.empty()) {
            throw NetError("empty result from RPC server");
        }
        char const* psz = reinterpret_cast<char const*>(received_data.data());
        UniValue res;
        res.read(psz, received_data.size());

        // Build result and return
        PLOG_DEBUG << "received: `" << res.write() << "`";
        Result result;
        if (res.exists("result")) {
            result.result = res["result"];
        }
        if (res.exists("error") && !res["error"].isNull()) {
            UniValue errorJson = res["error"];
            int code = errorJson["code"].get_int();
            std::string msg = errorJson["message"].get_str();
            throw RPCError(code, msg);
        }
        if (res.exists("id") && res["id"].isNum()) {
            result.id = res["id"].get_int();
        }
        return result;
    }

private:
    bool m_no_proxy;
    std::string m_wallet_name;
    std::string m_cookie_path_str;
    std::string m_url;
    std::string m_user;
    std::string m_passwd;
};

}  // namespace miner

#endif
