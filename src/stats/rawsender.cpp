// Copyright (c) 2017-2023 Vincent Thiery
// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stats/rawsender.h>

#include <logging.h>
#include <netaddress.h>
#include <netbase.h>
#include <util/sock.h>
#include <util/thread.h>

namespace {
/** Growth factor for timeout between each failed reconnection attempt (TCP only) */
constexpr uint8_t RECONNECT_TIMEOUT_GROWTH{2};
/** Maximum time taken between reconnection attempts (TCP only) */
constexpr std::chrono::seconds RECONNECT_TIMEOUT_MAX{5min};
} // anonymous namespace

RawSender::RawSender(const std::string& host, uint16_t port, bool use_tcp, std::pair<uint64_t, uint8_t> batching_opts,
                     uint64_t interval_ms, std::optional<bilingual_str>& error) :
    m_host{host},
    m_port{port},
    m_batching_opts{batching_opts},
    m_interval_ms{interval_ms},
    m_use_tcp{use_tcp}
{
    if (host.empty()) {
        error = _("No host specified");
        return;
    }

    if (auto error_opt = WITH_LOCK(cs_net, return Connect())) {
        error = *error_opt;
        return;
    }

    if (m_interval_ms == 0) {
        LogPrintf("Send interval is zero, not starting RawSender queueing thread.\n");
    } else {
        m_interrupt.reset();
        m_thread = std::thread(&util::TraceThread, "rawsender", [this] { QueueThreadMain(); });
    }

    if (m_use_tcp) {
        m_reconn = std::thread(&util::TraceThread, "rawreconnect", [this] { ReconnectThread(); });
    }

    LogPrintf("Started %sRawSender sending messages to %s over %s\n", m_thread.joinable() ? "threaded " : "",
              this->ToStringHostPort(), m_use_tcp ? "TCP" : "UDP");
}

RawSender::~RawSender()
{
    // If there are threads, interrupt and stop it
    if (m_thread.joinable()) {
        m_interrupt();
        m_thread.join();
    }
    if (m_reconn.joinable()) {
        m_reconn_interrupt();
        m_reconn.join();
    }
    // Flush queue of uncommitted messages
    QueueFlush(m_reconn_queue);
    QueueFlush(m_queue);

    LogPrintf("Stopped RawSender instance sending messages to %s:%d. %d successes, %d failures.\n",
              m_host, m_port, m_successes, m_failures);
}

std::optional<bilingual_str> RawSender::Connect()
{
    AssertLockHeld(cs_net);

    CService service{};
    if (auto netaddr = LookupHost(m_host, /*fAllowLookup=*/true); netaddr.has_value()) {
        if (!netaddr->IsIPv4() && !netaddr->IsIPv6()) {
            return strprintf(_("Host %s on unsupported network"), m_host);
        }
        service = CService(*netaddr, m_port);
        if (!service.GetSockAddr(reinterpret_cast<struct sockaddr*>(&m_server.first), &m_server.second)) {
            return strprintf(_("Cannot get socket address for %s"), this->ToStringHostPort());
        }
    } else {
        return strprintf(_("Unable to lookup host %s"), m_host);
    }

    if (!m_use_tcp) {
        SOCKET hSocket = ::socket(reinterpret_cast<struct sockaddr*>(&m_server.first)->sa_family, SOCK_DGRAM, IPPROTO_UDP);
        if (hSocket == INVALID_SOCKET) {
            return strprintf(_("Cannot create socket (::socket() returned error %s)"),
                             NetworkErrorString(WSAGetLastError()));
        }
        m_sock = std::make_unique<Sock>(hSocket);
    } else {
        // Connection could fail but this isn't catastrophic, reconnection thread will attempt again
        if (auto sock = ConnectDirectly(service, /*manual_connection=*/true); sock != nullptr) {
            m_sock = std::move(sock);
        }
    }

    return std::nullopt;
}

void RawSender::Reconnect()
{
    AssertLockHeld(cs_net);

    assert(m_use_tcp && !m_sock);

    m_reconn_stats.m_attempts++;
    LogPrintf("%s: Attempt %d at reconnecting with %s\n", __func__, m_reconn_stats.m_attempts, ToStringHostPort());

    // Connect() will not emit an error if connection failed, need to check for m_sock instead
    Connect();
    if (!m_sock) {
        // Connection attempt failed, increase timeout.
        m_reconn_stats.m_timeout = std::min(m_reconn_stats.m_timeout * RECONNECT_TIMEOUT_GROWTH, RECONNECT_TIMEOUT_MAX);
    } else {
        // No error reported, connection successful. Reset stats.
        m_reconn_stats = ReconnectionStats{};
    }
}

void RawSender::ReconnectThread()
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_net);

    while (!m_reconn_interrupt) {
        std::deque<RawMessage> queue{};
        {
            LOCK(cs_net);
            if (!m_sock) {
                Reconnect();
                if (!m_sock) {
                    LogPrint(BCLog::NET, "%s: Unable to establish connection with %s, will try again in %s seconds\n",
                             __func__, ToStringHostPort(), count_seconds(m_reconn_stats.m_timeout));
                } else {
                    LOCK(cs);
                    LogPrint(BCLog::NET, "%s: Successfully reconnected with %s\n", __func__, ToStringHostPort());
                    if (!m_reconn_queue.empty()) {
                        m_reconn_queue.swap(queue);
                    }
                }
            }
        }
        if (!queue.empty()) {
            QueueFlush(queue);
            LogPrintf("%s: Attempted to send %zu pending messages to %s\n", __func__, queue.size(), ToStringHostPort());
        }
        if (!m_reconn_interrupt.sleep_for(WITH_LOCK(cs_net, return m_reconn_stats.m_timeout))) {
            return;
        }
    }
}

std::optional<bilingual_str> RawSender::Send(const RawMessage& msg)
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_net);

    // If there is a thread, append to queue
    if (m_thread.joinable()) {
        WITH_LOCK(cs, QueueAdd(m_queue, msg));
        return std::nullopt;
    }
    // There isn't a queue, send directly
    return SendDirectly(msg);
}

std::optional<bilingual_str> RawSender::SendDirectly(const RawMessage& msg)
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_net);

    LOCK(cs_net);

    constexpr int send_flags{MSG_NOSIGNAL | MSG_DONTWAIT};
    if (m_use_tcp) {
        if (!m_sock) {
            // Not connected, add to message queue and let thread sort it out.
            WITH_LOCK(cs, QueueAdd(m_reconn_queue, msg));
            return std::nullopt;
        }

        if (m_sock->Send(reinterpret_cast<const char*>(msg.data()), msg.size(), send_flags) == SOCKET_ERROR) {
            const auto err_code = WSAGetLastError();
            m_failures++;
            if (err_code == WSAECONNABORTED ||
                err_code == WSAECONNREFUSED ||
                err_code == WSAECONNRESET ||
                err_code == WSAEHOSTUNREACH ||
                err_code == WSAENETDOWN ||
                err_code == WSAENETRESET ||
                err_code == WSAENETUNREACH ||
                err_code == WSAENOTCONN ||
#ifndef WIN32
                err_code == EPIPE ||
#endif // WIN32
                err_code == WSAETIMEDOUT)
            {
                // Reset socket to trigger reconnection, add to message queue.
                m_sock.reset();
                WITH_LOCK(cs, QueueAdd(m_reconn_queue, msg));
                return std::nullopt;
            }
            return strprintf(_("Unable to send message to %s (::send() returned error %s)"), this->ToStringHostPort(),
                             NetworkErrorString(err_code));
        }
    } else {
        if (!m_sock) {
            // UDP is connectionless, just bail out.
            m_failures++;
            return _("Socket not initialized, cannot send message");
        }

        if (::sendto(m_sock->Get(), reinterpret_cast<const char*>(msg.data()),
#ifdef WIN32
                     static_cast<int>(msg.size()),
#else
                     msg.size(),
#endif // WIN32
                     send_flags, reinterpret_cast<struct sockaddr*>(&m_server.first), m_server.second) == SOCKET_ERROR) {
            m_failures++;
            return strprintf(_("Unable to send message to %s (::sendto() returned error %s)"), this->ToStringHostPort(),
                             NetworkErrorString(WSAGetLastError()));
        }
    }

    m_successes++;
    return std::nullopt;
}

std::string RawSender::ToStringHostPort() const { return strprintf("%s:%d", m_host, m_port); }

void RawSender::QueueAdd(std::deque<RawMessage>& queue, const RawMessage& msg)
{
    AssertLockHeld(cs);

    const auto& [batch_size, batch_delim] = m_batching_opts;
    // If no batch size has been specified, simply add to queue
    if (batch_size == 0) {
        queue.push_back(msg);
        return;
    }

    // We can batch, either create a new batch in queue or append to existing batch in queue
    if (queue.empty() || queue.back().size() + msg.size() >= batch_size) {
        // Either we don't have a place to batch our message or we exceeded the batch size, make a new batch
        queue.emplace_back();
        queue.back().reserve(batch_size);
    } else if (!queue.back().empty()) {
        // When there is already a batch open we need a delimiter when its not empty
        queue.back() += batch_delim;
    }

    // Add the new message to the batch
    queue.back() += msg;
}

void RawSender::QueueFlush(std::deque<RawMessage>& queue)
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_net);

    for (auto& msg : queue) {
        // Add delimiter to prevent unexpected concat if sends are consolidated
        if (m_use_tcp && !msg.empty() && msg.back() != m_batching_opts.second) {
            msg += m_batching_opts.second;
        }
        SendDirectly(msg);
    }
}

void RawSender::QueueThreadMain()
{
    AssertLockNotHeld(cs);
    AssertLockNotHeld(cs_net);

    while (!m_interrupt) {
        // Swap the queues to commit the existing queue of messages
        std::deque<RawMessage> queue;
        WITH_LOCK(cs, m_queue.swap(queue));
        QueueFlush(queue);

        if (!m_interrupt.sleep_for(std::chrono::milliseconds(m_interval_ms))) {
            return;
        }
    }
}
