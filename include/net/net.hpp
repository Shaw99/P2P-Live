/**
* \file net.hpp
* \author kadds (itmyxyf@gmail.com)
* \brief This header file contains all socket header files on different platforms. Including fixed-length integer
* \version 0.1
* \date 2020-03-13
*
* @copyright Copyright (c) 2020.
This file is part of P2P-Live.

P2P-Live is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

P2P-Live is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with P2P-Live. If not, see <http: //www.gnu.org/licenses/>.
*
*/
#pragma once
#ifndef OS_WINDOWS
/// linux headers
#include <arpa/inet.h>
#include <cstddef>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#else
/// windows headers
#endif
#include "net_exception.hpp"

typedef unsigned char byte;

namespace net
{
using i64 = int64_t;
using u64 = uint64_t;
using i32 = int32_t;
using u32 = uint32_t;
using i16 = int16_t;
using u16 = uint16_t;
using i8 = int8_t;
using u8 = uint8_t;

/// Called before run event context
void init_lib();

/// Called when the application is closed
void uninit_lib();

enum io_result
{
    ok,
    /// io in process
    /// acceptor
    in_process,
    /// continue io request
    cont,
    /// connection is closed by peer/self
    closed,
    /// io request is timeout
    /// connector
    timeout,
    /// io request failed
    failed,
    /// buffer too small
    /// when udp recv
    buffer_too_small,
};
} // namespace net
