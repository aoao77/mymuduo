#pragma once

#include <memory>
#include <functional>

class Buffer;
class Timestamp;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void (const TcpConnection&)>;
using CloseCallback = std::function<void (const TcpConnection&)>;
using WriteCompleteCallback = std::function<void (const TcpConnection&)>;
using MessageCallback = std::function<void (const TcpConnection&,
                                        Buffer*,
                                        Timestamp)>;
using HighWaterMarkCallback = std::function<void (const TcpConnection&, size_t)>;