#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

#ifdef __POSIX__
#include <fcntl.h>
using posix_shm_info = struct stat;
using shm_info_t = posix_shm_info;
using shmsz_t = off_t;

#endif

#ifdef __WIN32__
#include <Windows.h>
using shmsz_t = unsigned long long;
#endif

namespace ipc {
#ifdef __POSIX__
enum class O_FLAGS {
  CREATE_ONLY = O_RDWR | O_CREAT | O_EXCL,
  OPEN_ONLY = O_RDWR,
  CREATE_OR_OPEN = O_RDWR | O_CREAT,
  WRITE_ONLY = O_WRONLY,
  READ_ONLY = O_RDONLY,
};

// shared memory object permission
enum class PERM {
  READ = S_IRUSR | S_IRGRP | S_IROTH,
  WRITE = S_IWUSR | S_IWGRP | S_IWOTH,
  EXEC = S_IXUSR | S_IXGRP | S_IXOTH,
  ALL = READ | WRITE | EXEC
};

#endif
} // namespace ipc
