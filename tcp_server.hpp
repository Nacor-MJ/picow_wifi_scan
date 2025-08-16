#pragma once
// tcp_server.hpp - C++ wrapper library for the lwIP TCP server example
// Converted from Raspberry Pi Pico C example to a small C++ class-based library.

#include <cstdint>
#include <array>
#include <cstdbool>

extern "C"
{
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
}

namespace pico_tcp
{

    constexpr uint16_t TCP_PORT = 4242;
    constexpr size_t BUF_SIZE = 2048;
    constexpr int TEST_ITERATIONS = 10;
    constexpr int POLL_TIME_S = 5;

    class TcpServer
    {
    public:
        TcpServer() = delete;
        explicit TcpServer(struct netif *netif = nullptr);
        ~TcpServer();

        // non-copyable
        TcpServer(const TcpServer &) = delete;
        TcpServer &operator=(const TcpServer &) = delete;

        // Start listening. Returns true on success.
        bool start();

        // Close server immediately.
        err_t close();

        // Check if test completed (success or fail).
        bool is_complete() const { return complete_; }

        // For the main loop (optional): returns true if server is valid/running.
        bool running() const { return server_pcb_ != nullptr; }

        // Get last result (0 == success, nonzero == failure). Only valid after complete.
        int last_status() const { return last_status_; }

    private:
        // Instance state (mirrors original struct)
        struct tcp_pcb *server_pcb_;
        struct tcp_pcb *client_pcb_;
        bool complete_;
        int last_status_;
        std::array<uint8_t, BUF_SIZE> buffer_sent_;
        std::array<uint8_t, BUF_SIZE> buffer_recv_;
        int sent_len_;
        int recv_len_;
        int run_count_;
        struct netif *netif_; // optional pointer for logging ip

        // Private helpers
        void result_and_close(int status);
        err_t send_data(struct tcp_pcb *tpcb);
        err_t close_client();

        // C-style callback wrappers (must be static)
        static err_t accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err);
        static err_t recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
        static err_t sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
        static err_t poll_cb(void *arg, struct tcp_pcb *tpcb);
        static void err_cb(void *arg, err_t err);
    };
} // namespace pico_tcp
