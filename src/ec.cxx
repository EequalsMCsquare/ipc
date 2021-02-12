#include <ec.hpp>

struct IPCErrorCategory : public std::error_category {
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const char*
IPCErrorCategory::name() const noexcept {
  return "ipc";
}

std::string IPCErrorCategory::message(int ev) const {
  switch (static_cast<IPCErrc>(ev)) {
  case IPCErrc::NoError:
    return "no error";
  case IPCErrc::ShmNotMapped:
    return "shm is not mapped!";
  case IPCErrc::ShmAddrNullptr:
    return "shm address is nullptr!";
  case IPCErrc::ShmDeleted:
      return "shm is marked as deleted!";
  default:
    return "unknown error";
  }
}

const IPCErrorCategory TheIPCErrorCategory{};

std::error_code make_error_code(IPCErrc ec) {
  return {static_cast<int>(ec), TheIPCErrorCategory};
}
