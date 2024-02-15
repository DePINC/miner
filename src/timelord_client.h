#ifndef TIMELORD_CLIENT_H
#define TIMELORD_CLIENT_H

#include <functional>

#include <vector>
#include <deque>
#include <map>

#include <asio.hpp>
using asio::ip::tcp;

using error_code = asio::error_code;

#include <uint256.h>

#include <bhd_types.h>

class UniValue;

class FrontEndClient : public std::enable_shared_from_this<FrontEndClient> {
public:
    enum class ErrorType { CONN, READ, WRITE };
    enum class Status { READY, CONNECTING, CONNECTED, CLOSED };

    using ConnectionHandler = std::function<void()>;
    using MessageHandler = std::function<void(UniValue const&)>;
    using ErrorHandler = std::function<void(ErrorType err_type, std::string const& errs)>;

    explicit FrontEndClient(asio::io_context& ioc);

    ~FrontEndClient();

    void Connect(std::string const& host, unsigned short port, ConnectionHandler conn_handler,
                 MessageHandler msg_handler, ErrorHandler err_handler);

    bool SendMessage(UniValue const& msg);

    void Exit();

private:
    void DoReadNext();

    void DoSendNext();

    asio::io_context& ioc_;
    std::atomic<Status> st_{Status::READY};
    tcp::socket s_;
    asio::streambuf read_buf_;
    std::vector<uint8_t> send_buf_;
    std::deque<std::string> sending_msgs_;
    ConnectionHandler conn_handler_;
    MessageHandler msg_handler_;
    ErrorHandler err_handler_;
};

struct ProofDetail {
    Bytes y;
    Bytes proof;
    uint8_t witness_type;
    uint64_t iters;
    int duration;
};

using ProofReceiver = std::function<void(uint256 const& challenge, ProofDetail const& detail)>;

class TimelordClient : public std::enable_shared_from_this<TimelordClient> {
public:
    using ConnectionHandler = std::function<void()>;
    using ErrorHandler = std::function<void(FrontEndClient::ErrorType type, std::string const& errs)>;
    using MessageHandler = std::function<void(UniValue const& msg)>;

    static std::shared_ptr<TimelordClient> CreateTimelordClient(asio::io_context& ioc);

    ~TimelordClient();

    void SetConnectionHandler(ConnectionHandler conn_handler);

    void SetErrorHandler(ErrorHandler err_handler);

    void SetProofReceiver(ProofReceiver proof_receiver);

    void Calc(uint256 const& challenge, uint64_t iters, uint256 const& group_hash, uint64_t total_size,
              int interval_secs);

    void Connect(std::string const& host, unsigned short port);

    void Exit();

private:
    explicit TimelordClient(asio::io_context& ioc);

    void DoWriteNextPing();

    void DoWaitPong();

    asio::io_context& ioc_;
    std::shared_ptr<FrontEndClient> pclient_;
    std::map<int, MessageHandler> msg_handlers_;
    std::shared_ptr<asio::steady_timer> ptimer_sender_;
    asio::steady_timer timer_pingpong_;
    std::unique_ptr<asio::steady_timer> ptimer_waitpong_;
    ConnectionHandler conn_handler_;
    ErrorHandler err_handler_;
    ProofReceiver proof_receiver_;
};

#endif
