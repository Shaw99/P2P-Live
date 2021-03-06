#include "net/tcp.hpp"
#include "net/socket.hpp"
#include <functional>
#include <iostream>

namespace net::tcp
{

co::async_result_t<io_result> connection_t::awrite(co::paramter_t &param, socket_buffer_t &buffer)
{
    return socket_awrite(param, socket, buffer);
}

co::async_result_t<io_result> connection_t::aread(co::paramter_t &param, socket_buffer_t &buffer)
{
    return socket_aread(param, socket, buffer);
}

/// wait next packet and read tcp application head
co::async_result_t<io_result> connection_t::aread_packet_head(co::paramter_t &param, package_head_t &head)
{
    if (param.get_user_ptr() == 0) /// version none, first read
    {
        socket_buffer_t buffer = socket_buffer_t::from_struct(head);
        /// first read version bytes
        buffer.expect().length(sizeof(head.version));

        auto res = socket_aread(param, socket, buffer);
        if (!res.is_finish())
        {
            return {};
        }
        if (res() != io_result::ok)
            return res();
        if (head.version > 4 || head.version < 1) /// unknown header version
            return io_result::failed;

        /// save state, and we don't execute this branch any more.
        param.set_user_ptr((void *)1);
    }
    // read rest head

    socket_buffer_t buffer = socket_buffer_t::from_struct(head);
    int length;
    assert(head.version <= 4 && head.version >= 1);
    switch (head.version)
    {
        case 1:
            length = sizeof(head.v1);
            break;
        case 2:
            length = sizeof(head.v2);
            break;
        case 3:
            length = sizeof(head.v3);
            break;
        case 4:
            length = sizeof(head.v4);
            break;
        default:
            break;
    }
    buffer.expect().length(length);         /// set size we expect
    buffer.walk_step(sizeof(head.version)); /// save to buffer at offset 1.

    auto res = socket_aread(param, socket, buffer);
    if (res.is_finish())
    {
        if (res() == io_result::ok)
        {
            buffer.get_base_ptr()[0] = head.version;
            buffer.expect().origin_length();
            switch (head.version)
            {
                case 1:
                    endian::cast_inplace(head.v1, buffer);
                    break;
                case 2:
                    endian::cast_inplace(head.v2, buffer);
                    break;
                case 3:
                    endian::cast_inplace(head.v3, buffer);
                    break;
                case 4:
                    endian::cast_inplace(head.v4, buffer);
                    break;
                default:
                    break;
            }
        }
    }
    return res;
}

co::async_result_t<io_result> connection_t::aread_packet_content(co::paramter_t &param, socket_buffer_t &buffer)
{
    return socket_aread(param, socket, buffer);
}

template <typename E>
co::async_result_t<io_result> set_head_and_send(co::paramter_t &param, E &head, u64 buf_size, socket_t *socket)
{
    int head_buffer_size = sizeof(E);
    head.size = buf_size;
    socket_buffer_t head_buffer(head_buffer_size);

    head_buffer.expect().origin_length();
    endian::save_to(head, head_buffer);
    auto ret = socket_awrite(param, socket, head_buffer);
    if (ret.is_finish())
    {
        if (ret() == io_result::ok)
        {
            param.set_user_ptr((void *)1);
            return ret;
        }
    }
    return ret;
}

co::async_result_t<io_result> connection_t::awrite_packet(co::paramter_t &param, package_head_t &head,
                                                          socket_buffer_t &buffer)
{
send_head:
    if (param.get_user_ptr() == 0)
    {
        int head_buffer_size;
        switch (head.version)
        {
            case 1: {
                auto ret = set_head_and_send(param, head.v1, buffer.get_length(), socket);
                if (ret.is_finish())
                {
                    if (ret() != io_result::ok)
                        return ret;
                    break;
                }
            }
                return {};
            case 2: {
                auto ret = set_head_and_send(param, head.v2, buffer.get_length(), socket);
                if (ret.is_finish())
                {
                    if (ret() != io_result::ok)
                        return ret;
                    break;
                }
            }
                return {};
            case 3: {
                auto ret = set_head_and_send(param, head.v3, buffer.get_length(), socket);
                if (ret.is_finish())
                {
                    if (ret() != io_result::ok)
                        return ret;
                    break;
                }
            }
                return {};
            case 4: {
                auto ret = set_head_and_send(param, head.v4, buffer.get_length(), socket);
                if (ret.is_finish())
                {
                    if (ret() != io_result::ok)
                        return ret;
                    break;
                }
            }
                return {};
            default:
                return io_result::failed;
        }
    }

send_data:

    return socket_awrite(param, socket, buffer);
}

co::async_result_t<io_result> conn_awrite(co::paramter_t &param, connection_t conn, socket_buffer_t &buffer)
{
    return conn.awrite(param, buffer);
}
co::async_result_t<io_result> conn_aread(co::paramter_t &param, connection_t conn, socket_buffer_t &buffer)
{
    return conn.aread(param, buffer);
}
co::async_result_t<io_result> conn_aread_packet_head(co::paramter_t &param, connection_t conn, package_head_t &head)
{
    return conn.aread_packet_head(param, head);
}
co::async_result_t<io_result> conn_aread_packet_content(co::paramter_t &param, connection_t conn,
                                                        socket_buffer_t &buffer)
{
    return conn.aread_packet_content(param, buffer);
}
co::async_result_t<io_result> conn_awrite_packet(co::paramter_t &param, connection_t conn, package_head_t &head,
                                                 socket_buffer_t &buffer)
{
    return conn.awrite_packet(param, head, buffer);
}

server_t::server_t()
    : server_socket(nullptr)
{
}

server_t::~server_t() { close_server(); }

void server_t::client_main(socket_t *socket)
{
    auto remote = socket->remote_addr();
    try
    {
        if (join_handler)
            join_handler(*this, socket);
    } catch (net::net_connect_exception &e)
    {
        if (error_handler)
            error_handler(*this, socket, remote, e.get_state());
    }
    exit_client(socket);
}

void server_t::wait_client()
{
    while (1)
    {
        auto socket = co::await(accept_from, server_socket);
        socket->bind_context(*context);
        socket->run(std::bind(&server_t::client_main, this, socket));
        socket->wake_up_thread();
    }
}

void server_t::listen(event_context_t &context, socket_addr_t address, int max_client, bool reuse_addr)
{
    if (server_socket != nullptr)
        return;
    this->context = &context;
    server_socket = new_tcp_socket();
    if (reuse_addr)
        reuse_addr_socket(server_socket, true);

    listen_from(bind_at(server_socket, address), max_client);

    server_socket->bind_context(context);
    server_socket->run(std::bind(&server_t::wait_client, this));
}

void server_t::exit_client(socket_t *client)
{
    assert(client != server_socket);

    if (!client)
        return;
    if (exit_handler)
        exit_handler(*this, client);

    client->unbind_context();
    close_socket(client);
}

void server_t::close_server()
{
    if (!server_socket)
        return;
    server_socket->unbind_context();
    close_socket(server_socket);
    server_socket = nullptr;
}

server_t &server_t::on_client_join(handler_t handler)
{
    join_handler = handler;
    return *this;
}

server_t &server_t::on_client_exit(handler_t handler)
{
    exit_handler = handler;
    return *this;
}

server_t &server_t::on_client_error(error_handler_t handler)
{
    error_handler = handler;
    return *this;
}

client_t::client_t()
    : socket(nullptr)
{
}

client_t::~client_t() { close(); }

void client_t::wait_server(socket_addr_t address, microsecond_t timeout)
{
    auto ret = co::await_timeout(timeout, connect_to, socket, address);
    if (io_result::ok == ret)
    {
        try
        {
            if (join_handler)
                join_handler(*this, socket);
        } catch (net::net_connect_exception &e)
        {
            if (error_handler)
                error_handler(*this, socket, address, e.get_state());
        }
    }
    else
    {
        if (error_handler)
        {
            if (io_result::timeout == ret)
                error_handler(*this, socket, address, connection_state::timeout);
            else if (io_result::failed == ret)
                error_handler(*this, socket, address, connection_state::connection_refuse);
            else
                error_handler(*this, socket, address, connection_state::closed);
        }
    }
    close();
}

void client_t::connect(event_context_t &context, socket_addr_t address, microsecond_t timeout)
{
    if (socket != nullptr)
        return;
    this->context = &context;
    socket = new_tcp_socket();
    connect_addr = address;
    socket->bind_context(context);
    socket->run(std::bind(&client_t::wait_server, this, address, timeout));
    socket->wake_up_thread();
}

client_t &client_t::on_server_connect(handler_t handler)
{
    join_handler = handler;
    return *this;
}

client_t &client_t::on_server_disconnect(handler_t handler)
{
    exit_handler = handler;
    return *this;
}

client_t &client_t::on_server_error(error_handler_t handler)
{
    error_handler = handler;
    return *this;
}

void client_t::close()
{
    if (!socket)
        return;
    if (exit_handler)
        exit_handler(*this, socket);
    socket->unbind_context();

    close_socket(socket);
    socket = nullptr;
}

bool client_t::is_connect() const { return socket && socket->is_connection_alive(); }

} // namespace net::tcp
