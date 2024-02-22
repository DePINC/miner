#include "rpc_client.h"

#include <bls_key.h>
#include <utils.h>

#include <fstream>

#include <bhd_types.h>

namespace miner {

std::string DepositTermToString(DepositTerm term) {
    switch (term) {
        case DepositTerm::NoTerm:
            return "noterm";
        case DepositTerm::Term1:
            return "term1";
        case DepositTerm::Term2:
            return "term2";
        case DepositTerm::Term3:
            return "term3";
    }
    return "wrong_term_value";
}

DepositTerm DepositTermFromString(std::string const& str) {
    if (str == "noterm") {
        return DepositTerm::NoTerm;
    } else if (str == "term1") {
        return DepositTerm::Term1;
    } else if (str == "term2") {
        return DepositTerm::Term2;
    } else if (str == "term3") {
        return DepositTerm::Term3;
    }
    return DepositTerm::NoTerm;
}

RPCClient::RPCClient(bool no_proxy, std::string url, std::string const& cookie_path_str)
        : m_no_proxy(no_proxy), m_cookie_path_str(cookie_path_str), m_url(std::move(url)) {
    if (cookie_path_str.empty()) {
        throw std::runtime_error("cookie is empty, cannot connect to depinc core");
    }
    LoadCookie();
}

RPCClient::RPCClient(bool no_proxy, std::string url, std::string user, std::string passwd)
        : m_no_proxy(no_proxy), m_url(std::move(url)), m_user(std::move(user)), m_passwd(std::move(passwd)) {}

void RPCClient::SetWallet(std::string const& wallet_name) { m_wallet_name = wallet_name; }

void RPCClient::LoadCookie() {
    fs::path cookie_path(m_cookie_path_str);
    std::ifstream cookie_reader(cookie_path.string());
    if (!cookie_reader.is_open()) {
        std::stringstream ss;
        ss << "cannot open to read " << cookie_path;
        throw std::runtime_error(ss.str());
    }
    std::string auth_str;
    std::getline(cookie_reader, auth_str);
    if (auth_str.empty()) {
        throw std::runtime_error("cannot read auth string from `.cookie`");
    }
    auto pos = auth_str.find_first_of(':');
    std::string user_str = auth_str.substr(0, pos);
    std::string passwd_str = auth_str.substr(pos + 1);
    m_user = std::move(user_str);
    m_passwd = std::move(passwd_str);
}

std::string const& RPCClient::GetCookiePath() const { return m_cookie_path_str; }

bool RPCClient::CheckChiapos() {
    auto res = SendMethod(m_no_proxy, "checkchiapos");
    return res.result.get_bool();
}

RPCClient::Challenge RPCClient::QueryChallenge() {
    auto res = SendMethod(m_no_proxy, "querychallenge");
    Challenge ch;
    ch.challenge = uint256S(res.result["challenge"].get_str());
    ch.difficulty = res.result["difficulty"].get_int64();
    ch.prev_block_hash = uint256S(res.result["prev_block_hash"].get_str());
    ch.prev_block_height = res.result["prev_block_height"].get_int();
    ch.prev_vdf_iters = res.result["prev_vdf_iters"].get_int64();
    ch.prev_vdf_duration = res.result["prev_vdf_duration"].get_int64();
    ch.target_height = res.result["target_height"].get_int();
    ch.target_duration = res.result["target_duration"].get_int64();
    ch.filter_bits = res.result["filter_bits"].get_int();
    ch.base_iters = res.result["base_iters"].get_int();
    if (res.result.exists("vdf_proofs")) {
        auto vdf_proofs = res.result["vdf_proofs"];
        if (vdf_proofs.isArray()) {
            for (auto const& vdf_proof : vdf_proofs.getValues()) {
                VdfProof local_vdf_proof;
                local_vdf_proof.challenge = uint256S(vdf_proof["challenge"].get_str());
                local_vdf_proof.y = chiapos::MakeVDFForm(chiapos::BytesFromHex(vdf_proof["y"].get_str()));
                local_vdf_proof.proof = chiapos::BytesFromHex(vdf_proof["proof"].get_str());
                local_vdf_proof.witness_type = vdf_proof["witness_type"].get_int();
                local_vdf_proof.iters = vdf_proof["iters"].get_int64();
                local_vdf_proof.duration = vdf_proof["duration"].get_int();
                ch.vdf_proofs.push_back(std::move(local_vdf_proof));
            }
        }
    }
    return ch;
}

RPCClient::PledgeParams RPCClient::QueryNetspace() {
    auto res = SendMethod(m_no_proxy, "querynetspace");
    PledgeParams params;
    params.netCapacityTB = res.result["netCapacityTB"].get_int64();
    params.calculatedOnHeight = res.result["calculatedOnHeight"].get_int64();
    params.supplied = res.result["supplied"].get_int64();
    return params;
}

void RPCClient::SubmitProof(ProofPack const& proof_pack) {
    SendMethod(m_no_proxy, "submitproof", proof_pack.prev_block_hash, proof_pack.prev_block_height,
               proof_pack.pos.challenge, proof_pack.pos, proof_pack.farmer_sk, proof_pack.vdf, proof_pack.reward_dest);
}

chiapos::Bytes RPCClient::BindPlotter(std::string const& address, chiapos::SecreKey const& farmerSk, int spend_height) {
    auto res = SendMethod(m_no_proxy, "bindchiaplotter", address, farmerSk, std::to_string(spend_height));
    return chiapos::BytesFromHex(res.result.get_str());
}

std::vector<RPCClient::BindRecord> RPCClient::ListBindTxs(std::string const& address, int count, int skip,
                                                          bool include_watchonly, bool include_invalid) {
    auto res = SendMethod(m_no_proxy, "listbindplotters", count, skip, include_watchonly, include_invalid, address);
    if (!res.result.isArray()) {
        throw std::runtime_error("non-array value is received from core");
    }
    std::vector<BindRecord> records;
    for (auto const& entry : res.result.getValues()) {
        BindRecord rec;
        rec.tx_id = chiapos::BytesFromHex(entry["txid"].get_str());
        rec.address = entry["address"].get_str();
        rec.farmer_pk = entry["plotterId"].get_str();
        if (entry.exists("blockhash")) {
            rec.block_hash = chiapos::BytesFromHex(entry["blockhash"].get_str());
        }
        if (entry.exists("blockheight")) {
            rec.block_height = entry["blockheight"].get_int();
        } else {
            rec.block_height = 0;
        }
        rec.active = entry["active"].get_bool();
        rec.valid = entry["valid"].getBool();
        records.push_back(std::move(rec));
    }
    return records;
}

chiapos::Bytes RPCClient::Deposit(std::string const& address, int amount, DepositTerm term) {
    auto res = SendMethod(m_no_proxy, "sendpledgetoaddress", address, amount, "no comment", "no comment", false, false,
                          1, "UNSET", DepositTermToString(term));
    return chiapos::BytesFromHex(res.result.get_str());
}

chiapos::Bytes RPCClient::Withdraw(chiapos::Bytes const& tx_id) {
    auto res = SendMethod(m_no_proxy, "withdrawpledge", chiapos::BytesToHex(tx_id));
    return chiapos::BytesFromHex(res.result.get_str());
}

bool RPCClient::GenerateBurstBlocks(int count) {
    auto res = SendMethod(m_no_proxy, "generateburstblocks", count);
    return res.result.get_bool();
}

chiapos::Bytes RPCClient::RetargetPledge(chiapos::Bytes const& tx_id, std::string const& address) {
    auto res = SendMethod(m_no_proxy, "retargetpledge", tx_id, address);
    return chiapos::BytesFromHex(res.result.get_str());
}

RPCClient::MiningRequirement RPCClient::QueryMiningRequirement(std::string const& address) {
    auto res = SendMethod(m_no_proxy, "queryminingrequirement", address);
    MiningRequirement mining_requirement;
    auto summary = res.result["summary"];
    mining_requirement.address = summary["address"].get_str();
    mining_requirement.req = summary["require"].get_int64();
    mining_requirement.mined_count = summary["mined"].get_int();
    mining_requirement.total_count = summary["count"].get_int();
    mining_requirement.burned = summary["burned"].get_int64();
    mining_requirement.accumulate = summary["accumulate"].get_int64();
    mining_requirement.supplied = summary["supplied"].get_int64();
    mining_requirement.height = summary["height"].get_int();
    mining_requirement.calc_height = summary["calc-height"].get_int();
    // TODO mining_requirement.farmer_pk should be retrieved
    return mining_requirement;
}

bool RPCClient::SubmitVdfRequest(uint256 const& challenge, uint64_t iters) {
    auto res = SendMethod(m_no_proxy, "submitvdfrequest", challenge.GetHex(), iters);
    return res.result.get_bool();
}

void RPCClient::BuildRPCJson(UniValue& params, std::string const& val) { params.push_back(val); }

void RPCClient::BuildRPCJson(UniValue& params, chiapos::Bytes const& val) {
    params.push_back(chiapos::BytesToHex(val));
}

void RPCClient::BuildRPCJson(UniValue& params, bool val) { params.push_back(UniValue(val)); }

void RPCClient::BuildRPCJson(UniValue& params, uint256 const& val) { params.push_back(val.GetHex()); }

void RPCClient::BuildRPCJson(UniValue& params, PosProof const& proof) {
    UniValue val(UniValue::VOBJ);
    val.pushKV("challenge", proof.challenge.GetHex());
    val.pushKV("k", proof.k);
    val.pushKV("pool_pk_or_hash", chiapos::BytesToHex(chiapos::ToBytes(proof.pool_pk_or_hash)));
    val.pushKV("plot_type", static_cast<int>(chiapos::GetType(proof.pool_pk_or_hash)));
    val.pushKV("local_pk", chiapos::BytesToHex(chiapos::MakeBytes(proof.local_pk)));
    val.pushKV("proof", chiapos::BytesToHex(proof.proof));
    params.push_back(val);
}

void RPCClient::BuildRPCJson(UniValue& params, VdfProof const& proof) {
    UniValue val(UniValue::VOBJ);
    val.pushKV("challenge", proof.challenge.GetHex());
    val.pushKV("y", chiapos::BytesToHex(chiapos::MakeBytes(proof.y)));
    val.pushKV("proof", chiapos::BytesToHex(proof.proof));
    val.pushKV("iters", proof.iters);
    val.pushKV("witness_type", proof.witness_type);
    val.pushKV("duration", proof.duration);
    params.push_back(val);
}

void RPCClient::BuildRPCJsonWithParams(UniValue& out_params) {}

}  // namespace miner
