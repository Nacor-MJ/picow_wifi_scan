// tcp_server.cpp
#include "tcp_server.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace pico_tcp;

#define DEBUG_printf printf

TcpServer::TcpServer(struct netif *netif)
    : server_pcb_(nullptr),
      client_pcb_(nullptr),
      complete_(false),
      last_status_(-1),
      sent_len_(0),
      recv_len_(0),
      run_count_(0),
      netif_(netif)
{
    // buffers zero-initialised by default constructor of std::array? no guarantee for POD, but
    // we'll explicitly clear recv buffer for determinism:
    buffer_sent_.fill(0);
    buffer_recv_.fill(0);
}

TcpServer::~TcpServer()
{
    close();
}

bool TcpServer::start()
{
    if (server_pcb_)
    {
        DEBUG_printf("already started\n");
        return false;
    }

    if (netif_)
    {
        DEBUG_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_)), TCP_PORT);
    }
    else
    {
        DEBUG_printf("Starting server on port %u\n", TCP_PORT);
    }

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err)
    {
        DEBUG_printf("failed to bind to port %u (err %d)\n", TCP_PORT, err);
        tcp_close(pcb);
        return false;
    }

    server_pcb_ = tcp_listen_with_backlog(pcb, 1);
    if (!server_pcb_)
    {
        DEBUG_printf("failed to listen\n");
        tcp_close(pcb);
        return false;
    }

    // store pointer to this instance in tcp_arg
    tcp_arg(server_pcb_, this);
    tcp_accept(server_pcb_, &TcpServer::accept_cb);
    return true;
}

err_t TcpServer::close()
{
    // close client first
    err_t err = ERR_OK;
    if (client_pcb_)
    {
        tcp_arg(client_pcb_, nullptr);
        tcp_poll(client_pcb_, nullptr, 0);
        tcp_sent(client_pcb_, nullptr);
        tcp_recv(client_pcb_, nullptr);
        tcp_err(client_pcb_, nullptr);
        err = tcp_close(client_pcb_);
        if (err != ERR_OK)
        {
            DEBUG_printf("close failed %d, aborting client\n", err);
            tcp_abort(client_pcb_);
            err = ERR_ABRT;
        }
        client_pcb_ = nullptr;
    }
    if (server_pcb_)
    {
        tcp_arg(server_pcb_, nullptr);
        tcp_close(server_pcb_);
        server_pcb_ = nullptr;
    }
    return err;
}

void TcpServer::result_and_close(int status)
{
    last_status_ = status;
    if (status == 0)
    {
        DEBUG_printf("test success\n");
    }
    else
    {
        DEBUG_printf("test failed %d\n", status);
    }
    complete_ = true;
    close();
}

err_t TcpServer::send_data(struct tcp_pcb *tpcb)
{
    // fill buffer with random bytes
    for (size_t i = 0; i < BUF_SIZE; ++i)
    {
        buffer_sent_[i] = static_cast<uint8_t>(rand() & 0xFF);
    }
    sent_len_ = 0;
    DEBUG_printf("Writing %zu bytes to client\n", BUF_SIZE);

    err_t err = tcp_write(tpcb, buffer_sent_.data(), static_cast<u16_t>(BUF_SIZE), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
        DEBUG_printf("Failed to write data %d\n", err);
        result_and_close(-1);
        return err;
    }
    return ERR_OK;
}

err_t TcpServer::close_client()
{
    if (client_pcb_)
    {
        tcp_arg(client_pcb_, nullptr);
        tcp_poll(client_pcb_, nullptr, 0);
        tcp_sent(client_pcb_, nullptr);
        tcp_recv(client_pcb_, nullptr);
        tcp_err(client_pcb_, nullptr);
        err_t err = tcp_close(client_pcb_);
        if (err != ERR_OK)
        {
            tcp_abort(client_pcb_);
            client_pcb_ = nullptr;
            return ERR_ABRT;
        }
        client_pcb_ = nullptr;
        return ERR_OK;
    }
    return ERR_OK;
}

/* -----------------------
   CALLBACKS (static)
   ----------------------- */

err_t TcpServer::accept_cb(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    TcpServer *self = static_cast<TcpServer *>(arg);
    if (!self)
        return ERR_ARG;

    if (err != ERR_OK || newpcb == nullptr)
    {
        DEBUG_printf("Failure in accept\n");
        self->result_and_close(err);
        return ERR_VAL;
    }

    DEBUG_printf("Client connected\n");

    self->client_pcb_ = newpcb;
    tcp_arg(newpcb, self);
    tcp_sent(newpcb, &TcpServer::sent_cb);
    tcp_recv(newpcb, &TcpServer::recv_cb);
    tcp_poll(newpcb, &TcpServer::poll_cb, POLL_TIME_S * 2);
    tcp_err(newpcb, &TcpServer::err_cb);

    return self->send_data(self->client_pcb_);
}

err_t TcpServer::sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    TcpServer *self = static_cast<TcpServer *>(arg);
    if (!self)
        return ERR_ARG;
    DEBUG_printf("tcp_server_sent %u\n", len);
    self->sent_len_ += len;
    if (self->sent_len_ >= static_cast<int>(BUF_SIZE))
    {
        // expect reply
        self->recv_len_ = 0;
        DEBUG_printf("Waiting for buffer from client\n");
    }
    return ERR_OK;
}

err_t TcpServer::recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    TcpServer *self = static_cast<TcpServer *>(arg);
    if (!self)
        return ERR_ARG;

    if (!p)
    {
        self->result_and_close(-1);
        return ERR_OK;
    }

    if (p->tot_len > 0)
    {
        DEBUG_printf("tcp_server_recv %u/%d err %d\n", static_cast<unsigned>(p->tot_len), self->recv_len_, err);
        const uint16_t buffer_left = static_cast<uint16_t>(BUF_SIZE - self->recv_len_);
        uint16_t to_copy = p->tot_len > buffer_left ? buffer_left : p->tot_len;
        self->recv_len_ += pbuf_copy_partial(p, self->buffer_recv_.data() + self->recv_len_, to_copy, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    if (self->recv_len_ == static_cast<int>(BUF_SIZE))
    {
        if (memcmp(self->buffer_sent_.data(), self->buffer_recv_.data(), BUF_SIZE) != 0)
        {
            DEBUG_printf("buffer mismatch\n");
            self->result_and_close(-1);
            return ERR_OK;
        }
        DEBUG_printf("tcp_server_recv buffer ok\n");
        self->run_count_++;
        if (self->run_count_ >= TEST_ITERATIONS)
        {
            self->result_and_close(0);
            return ERR_OK;
        }
        return self->send_data(self->client_pcb_);
    }
    return ERR_OK;
}

err_t TcpServer::poll_cb(void *arg, struct tcp_pcb * /*tpcb*/)
{
    TcpServer *self = static_cast<TcpServer *>(arg);
    if (!self)
        return ERR_ARG;
    DEBUG_printf("tcp_server_poll_fn\n");
    // no response considered error in original example
    self->result_and_close(-1);
    return ERR_OK;
}

void TcpServer::err_cb(void *arg, err_t err)
{
    TcpServer *self = static_cast<TcpServer *>(arg);
    if (!self)
        return;
    if (err != ERR_ABRT)
    {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        self->result_and_close(err);
    }
}
