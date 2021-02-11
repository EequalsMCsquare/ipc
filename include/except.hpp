#pragma once 

#include <stdexcept>

#include "ec.hpp"

namespace ipc
{
  class IPCExcept : std::exception
  {
  private:
    std::string _M_What;

  public:
    IPCExcept(const IPCErrc ec);
    IPCExcept(const std::error_code ec);

    const char* what() const noexcept final override;
  }
}