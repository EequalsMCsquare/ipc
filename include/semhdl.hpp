#pragma once

#include <atomic>
#include <chrono>
#include <semaphore.h>
#include <string_view>

#include "common.hpp"

namespace ipc {

class semhdl {
private:
  std::string name_;
#ifdef __POSIX__
  sem_t *sema_;
#endif

#ifdef __WIN32__
  HANDLE hSemaphore;
#endif

public:
  /**
   * @brief create a new semahdl object
   *
   * @param name
   * @param value
   */
  semhdl(std::string_view name, const uint32_t value,
         std::error_code &ec) noexcept;
  /**
   * @brief open an existing semahdl object
   *
   * @param name
   */
  semhdl(std::string_view name, std::error_code &) noexcept;

  /**
   * @brief Destroy the semahdl object
   *
   */
  ~semhdl();

  void post(std::error_code& ec) noexcept;

  void wait(std::error_code& ec) noexcept;

  void try_wait(std::error_code& ec) noexcept;

  int value(std::error_code &ec) const noexcept;

  std::string_view name() const noexcept;

};
} // namespace ipc