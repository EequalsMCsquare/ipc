#include <ec.hpp>

struct IPCErrcCategory : public std::error_category {
  const char *name() const noexcept override;
  std::string message(int ev) const override;
}

const char *
IPCErrorCategory::name() const noexcept {
  return "ipc";
}

std::string IPCErrcCategory::message(int ev) const {
  switch (static_cast<IPCErrc>(ev)) {
  case IPCErrc::NoError:
    return "no error";
  case IPCErrc::ShmNotMapped:
    return "shm is not mapped!";
  case IPCErrc::ShmAddrNullptr:
    return "shm address is nullptr!";
  default:
    return "unknown error";
  }
}

const IPCErrorCategory TheIPCErrorCategory{};

std::error_code make_error_code(IPCErrc ec) {
  return {static_cast<int>(ec), TheIPCErrorCategory};
}
