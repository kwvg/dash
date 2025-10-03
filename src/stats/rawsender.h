// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STATS_RAWSENDER_H
#define BITCOIN_STATS_RAWSENDER_H

#include <compat/compat.h>
#include <sync.h>
#include <util/threadinterrupt.h>
#include <util/time.h>
#include <util/translation.h>

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Sock;

struct RawMessage : public std::vector<uint8_t>
{
    using parent_type = std::vector<value_type>;
    using parent_type::parent_type;

    explicit RawMessage(const std::string& data) : parent_type{data.begin(), data.end()} {}

    parent_type& operator+=(value_type rhs) { return append(rhs); }
    parent_type& operator+=(std::string::value_type rhs) { return append(rhs); }
    parent_type& operator+=(const parent_type& rhs) { return append(rhs); }
    parent_type& operator+=(const std::string& rhs) { return append(rhs); }

    parent_type& append(value_type rhs)
    {
        push_back(rhs);
        return *this;
    }
    parent_type& append(std::string::value_type rhs)
    {
        push_back(static_cast<value_type>(rhs));
        return *this;
    }
    parent_type& append(const parent_type& rhs)
    {
        insert(end(), rhs.begin(), rhs.end());
        return *this;
    }
    parent_type& append(const std::string& rhs)
    {
        insert(end(), rhs.begin(), rhs.end());
        return *this;
    }
};

class RawSender
{
public:
    RawSender(const std::string& host, uint16_t port, bool use_tcp, std::pair<uint64_t, uint8_t> batching_opts,
              uint64_t interval_ms, std::optional<bilingual_str>& error);
    ~RawSender();

    RawSender(const RawSender&) = delete;
    RawSender& operator=(const RawSender&) = delete;
    RawSender(RawSender&&) = delete;

    //! Request a message to be sent based on configuration (queueing, batching)
    std::optional<bilingual_str> Send(const RawMessage& msg) EXCLUSIVE_LOCKS_REQUIRED(!cs, !cs_net);

private:
    //! Send a message directly using ::send{,to}()
    std::optional<bilingual_str> SendDirectly(const RawMessage& msg) EXCLUSIVE_LOCKS_REQUIRED(!cs, !cs_net);

    //! Get target server address as string
    std::string ToStringHostPort() const;

    //! Attempt connection
    std::optional<bilingual_str> Connect() EXCLUSIVE_LOCKS_REQUIRED(cs_net);

    //! Attempt reconnection to server (TCP only)
    void Reconnect() EXCLUSIVE_LOCKS_REQUIRED(cs_net);

    //! Worker thread function to attempt reconnection (TCP only)
    void ReconnectThread() EXCLUSIVE_LOCKS_REQUIRED(!cs, !cs_net);

    //! Add message to queue
    void QueueAdd(std::deque<RawMessage>& queue, const RawMessage& msg) EXCLUSIVE_LOCKS_REQUIRED(cs);

    //! Send all messages in given queue and flush it
    void QueueFlush(std::deque<RawMessage>& queue) EXCLUSIVE_LOCKS_REQUIRED(!cs, !cs_net);

    //! Worker thread function if queueing is requested
    void QueueThreadMain() EXCLUSIVE_LOCKS_REQUIRED(!cs, !cs_net);

private:
    /* Mutex to protect network parameters */
    mutable Mutex cs_net;
    /* Socket used to communicate with host */
    std::unique_ptr<Sock> m_sock GUARDED_BY(cs_net){nullptr};
    /* Socket address containing host information */
    std::pair<struct sockaddr_storage, socklen_t> m_server GUARDED_BY(cs_net){{}, sizeof(struct sockaddr_storage)};
    /* Reconnection stats (TCP only) */
    struct ReconnectionStats {
        /* Reconnection attempt counter */
        uint8_t m_attempts{0};
        /* Time between reconnection attempts */
        std::chrono::seconds m_timeout{1s};
    } m_reconn_stats GUARDED_BY(cs_net);

    /* Mutex to protect (batches of) messages queue */
    mutable Mutex cs;
    /* Interrupt for queue processing thread */
    CThreadInterrupt m_interrupt;
    /* Interrupt for reconnection thread (TCP only) */
    CThreadInterrupt m_reconn_interrupt;
    /* Queue of (batches of) messages to be sent */
    std::deque<RawMessage> m_queue GUARDED_BY(cs);
    /* Thread that processes queue every m_interval_ms */
    std::thread m_thread;
    /* Reconnection attempt thread (TCP only) */
    std::thread m_reconn;
    /* Queue of messages to be sent when reconnection succeeds (TCP only) */
    std::deque<RawMessage> m_reconn_queue GUARDED_BY(cs);

    /* Hostname of server receiving messages */
    const std::string m_host;
    /* Port of server receiving messages */
    const uint16_t m_port;
    /* Batching parameters */
    const std::pair</*size=*/uint64_t, /*delimiter=*/uint8_t> m_batching_opts{0, 0};
    /* Time between queue thread runs (expressed in milliseconds) */
    const uint64_t m_interval_ms;
    /* Communicating over TCP if true (or UDP is false) */
    const bool m_use_tcp;
};

#endif // BITCOIN_STATS_RAWSENDER_H
