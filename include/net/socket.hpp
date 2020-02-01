#pragma once
#include "co.hpp"
#include "event.hpp"
#include "net.hpp"
#include "socket_addr.hpp"
#include "socket_buffer.hpp"
#include <queue>

namespace net
{
enum class socket_status
{
    none,
    closed,
    connected,
};

class socket_t
{
    int fd;
    socket_addr_t local;
    socket_addr_t remote;
    bool is_connection_closed;

    co::coroutine_t *co;
    event_type_t current_event;
    event_loop_t *loop;

    friend co::async_result_t<io_result> connect_to(socket_t *socket, socket_addr_t socket_to_addr, int timeout_ms);
    friend co::async_result_t<socket_t *> accept_from(socket_t *in);
    friend class event_loop_t;

  private:
    io_result write_async(socket_buffer_t &buffer);
    io_result read_async(socket_buffer_t &buffer);

    io_result write_pack(socket_buffer_t &buffer, socket_addr_t target);
    io_result read_pack(socket_buffer_t &buffer, socket_addr_t &target);

  public:
    socket_t(int fd);
    ~socket_t();
    socket_t(const socket_t &) = delete;
    socket_t &operator=(const socket_t &) = delete;

    co::async_result_t<io_result> awrite(socket_buffer_t &buffer);
    co::async_result_t<io_result> aread(socket_buffer_t &buffer);

    co::async_result_t<io_result> awrite_to(socket_buffer_t &buffer, socket_addr_t target);
    co::async_result_t<io_result> aread_from(socket_buffer_t &buffer, socket_addr_t &target);

    socket_addr_t local_addr();
    socket_addr_t remote_addr();

    void on_event(event_context_t &context, event_type_t type);

    void add_event(event_type_t type);
    void remove_event(event_type_t type);

    int get_raw_handle() const { return fd; }

    event_type_t get_current_event() const { return current_event; }

    bool is_connection_alive() const { return !is_connection_closed; }

    event_loop_t &get_event_loop() { return *loop; }

    void startup_coroutine(co::coroutine_t *co);
};

co::async_result_t<io_result> socket_awrite(socket_t *socket, socket_buffer_t &buffer);
co::async_result_t<io_result> socket_aread(socket_t *socket, socket_buffer_t &buffer);

co::async_result_t<io_result> socket_awrite_to(socket_t *socket, socket_buffer_t &buffer, socket_addr_t target);
co::async_result_t<io_result> socket_aread_from(socket_t *socket, socket_buffer_t &buffer, socket_addr_t &target);

socket_t *new_tcp_socket();
socket_t *new_udp_socket();

socket_t *reuse_socket(socket_t *, bool reuse);

co::async_result_t<io_result> connect_to(socket_t *socket, socket_addr_t socket_to_addr, int timeout_ms);
co::async_result_t<io_result> connect_udp(socket_t *socket, socket_addr_t socket_to_addr, int timeout_ms);

socket_t *bind_at(socket_t *socket, socket_addr_t socket_to_addr);
socket_t *listen_from(socket_t *socket, int max_count);
co::async_result_t<socket_t *> accept_from(socket_t *in);
void close_socket(socket_t *socket);

} // namespace net
