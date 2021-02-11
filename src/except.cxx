#include "except.hpp"
#include <fmt/format.h>

namespace ipc {
IPCExcept::IPCExcept(const IPCErrc ec) {
  std::error_code __tmp_ec = ec;

  this->_M_What = fmt::format("({}) {}", __tmp_ec.value(), __tmp_ec.message());
}

IPCExcept::IPCExcept(const std::error_code ec)
{
  this->_M_What = fmt::format("({}) {}", ec.value(), ec.message());
}

const char* 
IPCExcept::what() const noexcept
{
  return this->_M_What.c_str();
}
} // namespace ipc