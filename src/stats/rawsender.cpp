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

    CService service{};
    if (auto netaddr = LookupHost(m_host, /*fAllowLookup=*/true); netaddr.has_value()) {
        if (!netaddr->IsIPv4() && !netaddr->IsIPv6()) {
            error = strprintf(_("Host %s on unsupported network"), m_host);
            return;
        }
        service = CService(*netaddr, port);
        if (!service.GetSockAddr(reinterpret_cast<struct sockaddr*>(&m_server.first), &m_server.second)) {
            error = strprintf(_("Cannot get socket address for %s"), this->ToStringHostPort());
            return;
        }
    } else {
        error = strprintf(_("Unable to lookup host %s"), m_host);
        return;
    }

    if (!m_use_tcp) {
        SOCKET hSocket = ::socket(reinterpret_cast<struct sockaddr*>(&m_server.first)->sa_family, SOCK_DGRAM, IPPROTO_UDP);
        if (hSocket == INVALID_SOCKET) {
            error = strprintf(_("Cannot create socket (socket() returned error %s)"),
                              NetworkErrorString(WSAGetLastError()));
            return;
        }
        m_sock = std::make_unique<Sock>(hSocket);
    } else {
        if (auto sock = ConnectDirectly(service, /*manual_connection=*/true); sock != nullptr) {
            m_sock = std::move(sock);
        } else {
            error = strprintf(_("Cannot create socket for %s"), this->ToStringHostPort());
            return;
        }
    }

    if (m_interval_ms == 0) {
        LogPrintf("Send interval is zero, not starting RawSender queueing thread.\n");
    } else {
        m_interrupt.reset();
        m_thread = std::thread(&util::TraceThread, "rawsender", [this] { QueueThreadMain(); });
    }

    LogPrintf("Started %sRawSender sending messages to %s over %s\n", m_thread.joinable() ? "threaded " : "",
              this->ToStringHostPort(), m_use_tcp ? "TCP" : "UDP");
}

RawSender::~RawSender()
{
    // If there is a thread, interrupt and stop it
    if (m_thread.joinable()) {
        m_interrupt();
        m_thread.join();
    }
    // Flush queue of uncommitted messages
    QueueFlush(m_queue);

    LogPrintf("Stopped RawSender instance sending messages to %s:%d. %d successes, %d failures.\n",
              m_host, m_port, m_successes, m_failures);
}

std::optional<bilingual_str> RawSender::Send(const RawMessage& msg)
{
    AssertLockNotHeld(cs);

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

    if (!m_sock) {
        m_failures++;
        return _("Socket not initialized, cannot send message");
    }

    constexpr int send_flags{MSG_NOSIGNAL | MSG_DONTWAIT};
    if (m_use_tcp) {
        if (m_sock->Send(reinterpret_cast<const char*>(msg.data()), msg.size(), send_flags) == SOCKET_ERROR) {
            m_failures++;
            return strprintf(_("Unable to send message to %s (::send() returned error %s)"), this->ToStringHostPort(),
                             NetworkErrorString(WSAGetLastError()));
        }
    } else {
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
