#pragma once
#include "endian.hpp"
#include "net.hpp"
#include "tcp.hpp"
#include "timer.hpp"
#include <functional>
#include <unordered_map>

namespace net
{
#pragma pack(push, 1)

struct peer_get_server_request_t
{
    u16 version;
    u32 room_id;
    u32 key;
    using member_list_t = net::serialization::typelist_t<u16, u32, u32>;
};

struct peer_get_server_respond_t
{
    u16 version;
    u16 port;
    u32 ip_addr;
    u32 session_id;
    u8 state;
    using member_list_t = net::serialization::typelist_t<u16, u16, u32, u32, u8>;
};

struct pull_server_request_common_t
{
    u16 type;
    using member_list_t = net::serialization::typelist_t<u16>;
};

struct pull_inner_server_respond_t
{
    u32 connect_count;
    using member_list_t = net::serialization::typelist_t<u32>;
};

#pragma pack(pop)
namespace load_balance
{
class front_server_t;
using front_handler_t =
    std::function<bool(front_server_t &, const peer_get_server_request_t &, peer_get_server_respond_t &, socket_t *)>;

using server_join_handler_t = std::function<void(front_server_t &, socket_t *)>;
/// load balance server
class front_server_t
{
  private:
    front_handler_t handler;
    server_join_handler_t server_handler;
    tcp::server_t server, inner_server;

  public:
    void bind_inner(event_context_t &context, socket_addr_t addr, bool reuse_addr = false);
    void bind(event_context_t &context, socket_addr_t addr, bool reuse_addr = false);
    front_server_t &at_client_request(front_handler_t handler);
    front_server_t &at_inner_server_join(server_join_handler_t handler);
};

} // namespace load_balance
} // namespace net
