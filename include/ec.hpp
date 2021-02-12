#pragma once 

#include <system_error>

enum class IPCErrc
{
  NoError = 0,
  ShmNotMapped,
  ShmAddrNullptr,
  ShmDeleted,
};

namespace std
{
template<>
  struct is_error_code_enum<IPCErrc> : true_type
  {};
}

std::error_code make_error_code(IPCErrc);