// #include <chainparams.h>
// #include <chainparamsbase.h>

#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

#include <uint256.h>
#include <vdf_computer.h>

#include <asio.hpp>

#include <chrono>
#include <cstdint>
#include <cxxopts.hpp>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include "block_fields.h"
#include "pos.h"

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include <bhd_types.h>
#include <timelord_client.h>

#include <utils.h>
#include <bls_key.h>
#include <vdf.h>
#include <calc_diff.h>

#include <rpc_client.h>
#include <http_client.h>
#include <config.h>
#include <prover.h>
#include <tools.h>
#include <chiapos_miner.h>

const std::function<std::string(char const*)> G_TRANSLATION_FUN = nullptr;

namespace miner {

enum class CommandType : int {
    UNKNOWN,
    GEN_CONFIG,
    MINING,
    BIND,
    DEPOSIT,
    REGARGET,
    WITHDRAW,
    MINING_REQ,
    MAX
};

std::string ConvertCommandToString(CommandType type) {
    switch (type) {
        case CommandType::UNKNOWN:
            return "(unknown)";
        case CommandType::GEN_CONFIG:
            return "generate-config";
        case CommandType::MINING:
            return "mining";
        case CommandType::BIND:
            return "bind";
        case CommandType::DEPOSIT:
            return "deposit";
        case CommandType::REGARGET:
            return "retarget";
        case CommandType::WITHDRAW:
            return "withdraw";
        case CommandType::MINING_REQ:
            return "mining-req";
        case CommandType::MAX:
            return "(max)";
    }
    return "(unknown)";
}

int MaxOfCommands() { return static_cast<int>(CommandType::MAX); }

CommandType ParseCommandFromString(std::string const& str) {
    for (int i = 1; i < MaxOfCommands(); ++i) {
        auto cmd = static_cast<CommandType>(i);
        if (str == ConvertCommandToString(cmd)) {
            return cmd;
        }
    }
    return CommandType::UNKNOWN;
}

std::string GetCommandsList() {
    std::stringstream ss;
    for (int i = 1; i < MaxOfCommands(); ++i) {
        auto str = ConvertCommandToString(static_cast<CommandType>(i));
        if (i + 1 < MaxOfCommands()) {
            ss << str << ", ";
        } else {
            ss << str;
        }
    }
    return ss.str();
}

struct Arguments {
    std::string command;
    bool verbose;  // show debug logs
    bool help;
    bool valid_only;  // only show valid records
    // arguments for command `account`
    bool check;        // parameter to check status with commands `bind`, `deposit`
    int amount;        // set the amount to deposit
    int index;         // the index for seed
    int height;        // custom height for bind-tx
    DepositTerm term;  // The term those DePC should be locked on chain
    chiapos::Bytes tx_id;
    std::string address;
    // Network related
    int difficulty_constant_factor_bits;  // dcf bits (chain parameter)
    std::string datadir;                  // The root path of the data directory
    std::string cookie_path;              // The file stores the connecting information of current depinc server
    std::string posproofs_path;           // The pos proofs for testing timeing
} g_args;

miner::Config g_config;

std::map<chiapos::PubKey, chiapos::SecreKey> ConvertSecureKeys(std::vector<std::string> const& seeds) {
    std::map<chiapos::PubKey, chiapos::SecreKey> res;
    for (std::string const& seed : seeds) {
        chiapos::CWallet wallet(chiapos::CKey::CreateKeyWithMnemonicWords(seed, ""));
        auto sk = wallet.GetFarmerKey(0);
        res[sk.GetPubKey()] = sk.GetSecreKey();
    }
    for (auto const& sk_pair : res) {
        PLOGI << tinyformat::format("Read farmer public-key: %s",
                                    chiapos::BytesToHex(chiapos::MakeBytes(sk_pair.first)));
    }
    return res;
}

chiapos::CKey GetSelectedKeyFromSeeds() {
    if (miner::g_args.index >= miner::g_config.GetSeeds().size()) {
        throw std::runtime_error("arg `index` is out of range, check settings for your seeds to ensure it is correct");
    }
    chiapos::CWallet wallet(
            chiapos::CKey::CreateKeyWithMnemonicWords(miner::g_config.GetSeeds()[miner::g_args.index], ""));
    return wallet.GetFarmerKey(0);
}

}  // namespace miner

int HandleCommand_GenConfig(std::string const& config_path) {
    if (fs::exists(config_path)) {
        PLOG_ERROR << "the config file does already exist, if you want to generate a new one, please delete it first";
        return 1;
    }
    PLOG_INFO << "writing a empty config file: " << config_path;

    miner::Config config;
    std::ofstream out(config_path);
    if (!out.is_open()) {
        throw std::runtime_error("cannot write config");
    }
    out << config.ToJsonString();

    return 0;
}

int HandleCommand_Mining() {
    miner::Prover prover(miner::StrListToPathList(miner::g_config.GetPlotPath()), miner::g_config.GetAllowedKs());
    std::unique_ptr<miner::RPCClient> pclient = tools::CreateRPCClient(miner::g_config, miner::g_args.cookie_path);
    // Start mining
    miner::Miner miner(*pclient, prover, miner::ConvertSecureKeys(miner::g_config.GetSeeds()),
                       miner::g_config.GetRewardDest(), miner::g_args.difficulty_constant_factor_bits);
    // do we have timelord service
    auto timelord_endpoints = miner::g_config.GetTimelordEndpoints();
    miner.StartTimelord(timelord_endpoints, 19191);
    return miner.Run();
}

int HandleCommand_Bind() {
    std::unique_ptr<miner::RPCClient> pclient = tools::CreateRPCClient(miner::g_config, miner::g_args.cookie_path);
    if (miner::g_args.check) {
        auto txs = pclient->ListBindTxs(miner::g_config.GetRewardDest(), 99999, 0, true, true);
        int COLUMN_WIDTH{15};
        for (auto const& tx : txs) {
            std::cout << std::setw(COLUMN_WIDTH) << "--> txid: " << chiapos::BytesToHex(tx.tx_id) << std::endl
                      << std::setw(COLUMN_WIDTH) << "height: " << tx.block_height << std::endl
                      << std::setw(COLUMN_WIDTH) << "address: " << tx.address << std::endl
                      << std::setw(COLUMN_WIDTH) << "farmer: " << tx.farmer_pk << std::endl
                      << std::setw(COLUMN_WIDTH) << "valid: " << (tx.valid ? "yes" : "invalid") << std::endl
                      << std::setw(COLUMN_WIDTH) << "active: " << (tx.active ? "yes" : "inactive") << std::endl;
        }
        return 0;
    }
    chiapos::Bytes tx_id = pclient->BindPlotter(miner::g_config.GetRewardDest(),
                                                miner::GetSelectedKeyFromSeeds().GetSecreKey(), miner::g_args.height);
    PLOG_INFO << "tx id: " << chiapos::BytesToHex(tx_id);
    return 0;
}

int HandleCommand_Deposit() {
    std::unique_ptr<miner::RPCClient> pclient = tools::CreateRPCClient(miner::g_config, miner::g_args.cookie_path);
    auto challenge = pclient->QueryChallenge();
    auto current_height = challenge.target_height - 1;
    PLOG_INFO << "height: " << current_height;
    // auto params = miner::GetChainParams();
    // Deposit with amount
    chiapos::Bytes tx_id = pclient->Deposit(miner::g_config.GetRewardDest(), miner::g_args.amount, miner::g_args.term);
    PLOG_INFO << "tx id: " << chiapos::BytesToHex(tx_id);
    return 0;
}

int HandleCommand_Withdraw() {
    std::unique_ptr<miner::RPCClient> pclient = tools::CreateRPCClient(miner::g_config, miner::g_args.cookie_path);
    chiapos::Bytes tx_id = pclient->Withdraw(miner::g_args.tx_id);
    PLOG_INFO << "tx id: " << chiapos::BytesToHex(tx_id);
    return 0;
}

int HandleCommand_MiningRequirement() {
    std::unique_ptr<miner::RPCClient> pclient = tools::CreateRPCClient(miner::g_config, miner::g_args.cookie_path);
    auto req = pclient->QueryMiningRequirement(miner::g_config.GetRewardDest(),
                                               miner::GetSelectedKeyFromSeeds().GetPubKey());
    int const PREFIX_WIDTH = 14;
    std::cout << std::setw(PREFIX_WIDTH) << "address: " << std::setw(15) << req.address << std::endl;
    std::cout << std::setw(PREFIX_WIDTH) << "mined: " << std::setw(15)
              << tinyformat::format("%d/%d", req.mined_count, req.total_count) << " BLK" << std::endl;
    std::cout << std::setw(PREFIX_WIDTH) << "supplied: " << std::setw(15) << chiapos::MakeNumberStr(req.supplied / COIN)
              << " DePC" << std::endl;
    std::cout << std::setw(PREFIX_WIDTH) << "burned: " << std::setw(15) << chiapos::MakeNumberStr(req.burned / COIN)
              << " DePC" << std::endl;
    std::cout << std::setw(PREFIX_WIDTH) << "accumulate: " << std::setw(15)
              << chiapos::MakeNumberStr(req.accumulate / COIN) << " DePC" << std::endl;
    std::cout << std::setw(PREFIX_WIDTH) << "require: " << std::setw(15) << chiapos::MakeNumberStr(req.req / COIN)
              << " DePC" << std::endl;
    return 0;
}

struct SubsidyRecord {
    time_t start_time;
    int first_height;
    int last_height;
    CAmount total;
};

std::string TimeToDate(time_t t) {
    tm* local = localtime(&t);
    std::stringstream ss;
    ss << local->tm_year + 1900 << "-" << std::setw(2) << std::setfill('0') << local->tm_mon + 1 << "-" << std::setw(2)
       << std::setfill('0') << local->tm_mday;
    return ss.str();
}

int HandleCommand_Retarget() {
    std::unique_ptr<miner::RPCClient> pclient = tools::CreateRPCClient(miner::g_config, miner::g_args.cookie_path);
    auto tx_id = pclient->RetargetPledge(miner::g_args.tx_id, miner::g_args.address);
    PLOG_INFO << "Retarget pledge to address: " << miner::g_args.address << ", tx_id: " << chiapos::BytesToHex(tx_id);
    return 0;
}

struct ProofRecord {
    int height;
    chiapos::CPosProof pos;
    chiapos::CVdfProof vdf;
};

class BlockGeneratingSimulator {
public:
    BlockGeneratingSimulator(uint64_t const& init_diff, uint64_t ips) : init_diff_(init_diff), ips_(ips) {}

    uint64_t AdjustDifficulty(uint64_t curr_diff, chiapos::CPosProof const& pos, uint64_t duration,
                              uint64_t target_duration, double diff_change_max_factor, double targetMulFactor) {
        return chiapos::AdjustDifficulty(curr_diff, duration, target_duration, 0, diff_change_max_factor, init_diff_,
                                         targetMulFactor);
    }

    uint64_t CalculateIterations(chiapos::CPosProof const& pos, int bits_filter, uint64_t diff,
                                 uint8_t diff_factor_bits, int base_iters) {
        auto mixed_quality_str = chiapos::MakeMixedQualityString(
                chiapos::MakeArray<chiapos::PK_LEN>(pos.vchLocalPk),
                chiapos::MakeArray<chiapos::PK_LEN>(pos.vchFarmerPk),
                chiapos::MakePubKeyOrHash(static_cast<chiapos::PlotPubKeyType>(pos.nPlotType), pos.vchPoolPkOrHash),
                pos.nPlotK, pos.challenge, pos.vchProof);
        return chiapos::CalculateIterationsQuality(mixed_quality_str, diff, bits_filter, diff_factor_bits, pos.nPlotK,
                                                   base_iters);
    }

    int CalculateDurationByIterations(uint64_t iters) const { return iters / ips_; }

private:
    uint64_t init_diff_;
    uint64_t ips_;
};

template <typename T>
T MakeRandomInt() {
    int n = sizeof(T);
    auto bytes = std::unique_ptr<uint8_t>(new uint8_t[n]);
    for (int i = 0; i < n; ++i) {
        bytes.get()[i] = rand() % 256;
    }
    T r;
    memcpy(&r, bytes.get(), n);
    return r;
}

uint256 MakeRandomUint256() {
    int n = 256 / 64;
    std::unique_ptr<uint64_t> r(new uint64_t[n]);
    for (int i = 0; i < n; ++i) {
        r.get()[i] = MakeRandomInt<uint64_t>();
    }
    uint256 res;
    memcpy(res.begin(), r.get(), 256 / 8);
    return res;
}

int main(int argc, char** argv) {
    plog::ConsoleAppender<plog::TxtFormatter> console_appender;

    cxxopts::Options opts("depinc-miner", "DePINC miner - A mining program for DePINC, chia PoC consensus.");
    opts.add_options()                            // All options
            ("h,help", "Show help document")      // --help
            ("v,verbose", "Show debug logs")      // --verbose
            ("valid", "Show only valid records")  // --valid
            ("l,log", "The path to the log file, turn it of with an empty string",
             cxxopts::value<std::string>()->default_value("miner.log"))  // --log
            ("log-max_size", "The max size of each log file",
             cxxopts::value<int>()->default_value(std::to_string(1024 * 1024 * 10)))  // --log-max_size
            ("log-max_count", "How many log files should be saved",
             cxxopts::value<int>()->default_value("10"))  // --log-max_count
            ("c,config", "The config file stores all miner information",
             cxxopts::value<std::string>()->default_value("./config.json"))  // --config
            ("no-proxy", "Do not use proxy")                                 // --no-proxy
            ("check", "Check the account status")                            // --check
            ("term", "The term of those DePC will be locked on chain (noterm, term1, term2, term3)",
             cxxopts::value<std::string>()->default_value("noterm"))  // --term
            ("txid", "The transaction id, it should be provided with command: withdraw, retarget",
             cxxopts::value<std::string>()->default_value(""))                                 // --txid
            ("amount", "The amount to be deposit", cxxopts::value<int>()->default_value("0"))  // --amount
            ("index", "The index of the seed from seeds array parsed from config.json",
             cxxopts::value<int>()->default_value("0"))                                                 // --index
            ("address", "The address for retarget or related commands", cxxopts::value<std::string>())  // --address
            ("height", "The height to custom bind-tx or anything else",
             cxxopts::value<int>()->default_value("0"))  // --height
            ("dcf-bits", "Difficulty constant factor bits",
             cxxopts::value<int>()->default_value(
                     std::to_string(chiapos::DIFFICULTY_CONSTANT_FACTOR_BITS)))  // --dcf-bits
            ("d,datadir", "The root path of the data directory",
             cxxopts::value<std::string>())  // --datadir, -d
            ("cookie", "Full path to `.cookie` from depinc datadir",
             cxxopts::value<std::string>())                                                       // --cookie
            ("posproofs", "Path to the file contains PoS proofs", cxxopts::value<std::string>())  // --posproofs
            ("command", std::string("Command") + miner::GetCommandsList(),
             cxxopts::value<std::string>())  // --command
            ;

    opts.parse_positional({"command"});
    cxxopts::ParseResult result = opts.parse(argc, argv);
    if (result["help"].as<bool>()) {
        std::cout << opts.help() << std::endl;
        std::cout << "Commands (" << miner::GetCommandsList() << ")" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  You should use command `generate-config` to make a new blank config." << std::endl;
        return 0;
    }

    miner::g_args.verbose = result["verbose"].as<bool>();
    auto& logger = plog::init((miner::g_args.verbose ? plog::debug : plog::info), &console_appender);

    std::string log_path = result["log"].as<std::string>();
    log_path = result["log"].as<std::string>();
    if (!log_path.empty()) {
        int max_size = result["log-max_size"].as<int>();
        int max_count = result["log-max_count"].as<int>();
        static plog::RollingFileAppender<plog::TxtFormatter> rollingfile_appender(log_path.c_str(), max_size,
                                                                                  max_count);
        logger.addAppender(&rollingfile_appender);
    }

    PLOG_DEBUG << "Initialized log system";

    if (result.count("command")) {
        miner::g_args.command = result["command"].as<std::string>();
    } else {
        PLOGE << "no command, please use --help to read how to use the program.";
        return 1;
    }

    auto config_path = result["config"].as<std::string>();
    if (config_path.empty()) {
        PLOGE << "cannot find config file, please use `--config` to set one";
        return 1;
    }

    // we need to generate config before parsing it
    auto cmd = miner::ParseCommandFromString(miner::g_args.command);
    if (cmd == miner::CommandType::GEN_CONFIG) {
        try {
            return HandleCommand_GenConfig(config_path);
        } catch (std::exception const& e) {
            PLOGE << "error occurs when generating config: " << e.what();
            return 1;
        }
    }

    miner::g_args.check = result["check"].as<bool>();
    miner::g_args.valid_only = result["valid"].as<bool>();
    miner::g_args.amount = result["amount"].as<int>();
    miner::g_args.index = result["index"].as<int>();
    miner::g_args.term = miner::DepositTermFromString(result["term"].as<std::string>());
    if (result.count("txid")) {
        miner::g_args.tx_id = chiapos::BytesFromHex(result["txid"].as<std::string>());
    }

    if (result.count("address") > 0) {
        miner::g_args.address = result["address"].as<std::string>();
    }

    if (result.count("height") > 0) {
        miner::g_args.height = result["height"].as<int>();
    }

    try {
        miner::g_config = tools::ParseConfig(config_path);
    } catch (std::exception const& e) {
        PLOGE << "parse config error: " << e.what();
        return 1;
    }

    if (result.count("datadir")) {
        // Customized datadir
        miner::g_args.datadir = result["datadir"].as<std::string>();
    } else {
        miner::g_args.datadir = tools::GetDefaultDataDir(miner::g_config.Testnet());
    }

    if (result.count("cookie")) {
        miner::g_args.cookie_path = result["cookie"].as<std::string>();
    } else {
        Path cookie_path(miner::g_args.datadir);
        cookie_path /= ".cookie";
        if (fs::exists(cookie_path)) {
            miner::g_args.cookie_path = cookie_path.string();
        }
    }

    if (result.count("posproofs")) {
        miner::g_args.posproofs_path = result["posproofs"].as<std::string>();
    }

    miner::g_args.difficulty_constant_factor_bits = result["dcf-bits"].as<int>();

    PLOG_INFO << "network: " << (miner::g_config.Testnet() ? "testnet" : "main");

    try {
        switch (miner::ParseCommandFromString(miner::g_args.command)) {
            case miner::CommandType::MINING:
                return HandleCommand_Mining();
            case miner::CommandType::BIND:
                return HandleCommand_Bind();
            case miner::CommandType::DEPOSIT:
                return HandleCommand_Deposit();
            case miner::CommandType::WITHDRAW:
                return HandleCommand_Withdraw();
            case miner::CommandType::REGARGET:
                return HandleCommand_Retarget();
            case miner::CommandType::MINING_REQ:
                return HandleCommand_MiningRequirement();
            case miner::CommandType::GEN_CONFIG:
            case miner::CommandType::UNKNOWN:
            case miner::CommandType::MAX:
                break;
        }
        throw std::runtime_error(std::string("unknown command: ") + miner::g_args.command);
    } catch (std::exception const& e) {
        PLOG_ERROR << e.what();
        return 1;
    }
}
