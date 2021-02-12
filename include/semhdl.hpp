#pragma once

#include <atomic>
#include <chrono>
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
   * @param ec
   */
  semhdl(std::string_view name, const uint32_t value,
         std::error_code &ec) noexcept;
  semhdl(std::string_view name, const uint32_t value);

  /**
   * @brief open an existing semahdl object
   *
   * @param name
   */
  semhdl(std::string_view name, std::error_code &) noexcept;
  semhdl(std::string_view name);

  /**
   * @brief Destroy the semahdl object
   *
   */
  ~semhdl();

  /**
   * @brief increase semaphore value
   *
   * @param ec
   */
  void post(std::error_code &ec) noexcept;
  void post();

  /**
   * @brief block waiting for semaphore value decrease to 0, then unblock and
   * increase the semaphore value
   *
   * @param ec
   */
  void wait(std::error_code &ec) noexcept;
  void wait();
  /**
   * @brief non-block wiat
   *
   * @param ec
   */
  void try_wait(std::error_code &ec) noexcept;

  /**
   * @brief semaphore's value
   *
   * @param ec
   * @return int
   */
  int value(std::error_code &ec) const noexcept;
  int value() const;
  /**
   * @brief semaphore's name
   *
   * @return std::string_view
   */
  std::string_view name() const noexcept;
};
} // namespace ipc