#include "except.hpp"

namespace ipc {
IPCExcept::IPCExcept(const IPCErrc ec) {
  std::error_code __tmp_ec = ec;
  snprintf(this->_M_What, 256, "(%d) %s", __tmp_ec.value(), __tmp_ec.message().c_str());
}

IPCExcept::IPCExcept(const std::error_code ec)
{
  snprintf(this->_M_What ,256,"(%d) %s", ec.value(), ec.message().c_str());
}

const char* 
IPCExcept::what() const noexcept
{
  return this->_M_What;
}
} // namespace ipc