#pragma once

#include "common.hpp"
#include <atomic>
#include <string_view>

namespace ipc {
/**
 * @brief shared memory object status
 *
 */
enum class SHM_STATUS : size_t {
  /**
   * @brief status is ok
   * @details functions can be normally called
   *
   */
  OK = 0,
  /**
   * @brief status is marked as deleted
   * @details his means that user can no long call any function with that
   * handle. This is usually caused by one of the attached handles has called
   * unlink().
   *
   */
  DEL = 1,
};

class shmhdl {
private:
  /**
   * @brief shared memory meta info
   * @details the meta info will be store at the begining of the shared memory
   * object.
   * memory layout might look like this:
   *  | shm_status | ref_count | size | mutex| buffer |
   */
  struct shm_meta_t {
    SHM_STATUS status_;
    std::atomic_size_t ref_count_;
    shmsz_t shmsz_;
  };

#ifdef __POSIX__
  /**
   * @brief shared memory object file descriptor
   * @details this is only availible for POSIX supported platforms. User can use
   * it with posix APIs;
   */
  int fd_;
#endif

#ifdef __WIN32__
  HANDLE hMapFile_;
#endif

  /**
   * @brief shm_handle's name
   *
   */
  std::string name_;

  /**
   * @brief shared memory buffer ptr
   *
   */
  void *addr_;

  /**
   * @brief shared memory meta ptr
   *
   */
  shm_meta_t *meta_;

  void unmap_meta(std::error_code &ec) noexcept;

public:
  /**
   * @brief create a new shared memory object with given size
   *
   * @param name
   * @param nbytes
   * @param error_code
   */
  shmhdl(std::string_view name, const shmsz_t nbytes, std::error_code &ec);
  /**
   * @brief attach to a existing shared memory object
   *
   * @param name
   */
  shmhdl(std::string_view name, std::error_code &ec);
  shmhdl(const shmhdl &) = delete;
  ~shmhdl();

  /**
   * @brief map shared memory object into current process
   * @details if current handle already called map(), this function will just
   * return the ptr
   * @param ec
   * @return void*
   */
  void *map(std::error_code &ec) noexcept;
  /**
   * @brief unmap shared memory object from current process.
   *
   * @param ec
   */
  void unmap(std::error_code &ec) noexcept;
  /**
   * @brief unmap current shared memory object and remove the file in /dev/shm
   *
   * @param ec
   */
  void unlink(std::error_code &ec) noexcept;

  /**
   * @brief size of shared memory object (meta exclude)
   *
   * @return const shmsz_t
   */
  shmsz_t nbytes() const noexcept;
  /**
   * @brief shared memory object name
   *
   * @return std::string_view
   */
  std::string_view name() const noexcept;
  /**
   * @brief mapped address head of this shared memory object in current process
   *
   * @return void*
   */
  void *addr() const noexcept;
  /**
   * @brief Reference count of current shared memory object
   *
   * @return const size_t&
   */
  size_t ref_count() const noexcept;

#ifdef __WIN32__
  /**
   * @brief Win32 API's native shared memory object HANDLE
   *
   */
  HANDLE native_handle() noexcept
#endif

#ifdef __POSIX__

  /**
   * @brief unix file descriptor
   *
   * @return const int
   */
  int fd() const noexcept;
#endif
};
} // namespace ipc
