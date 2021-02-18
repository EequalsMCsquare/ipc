#include "shmhdl.hpp"

#include <cstdio>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace ipc {

void shmhdl::unmap_meta(std::error_code &ec) noexcept {
  ec.clear();
  munmap(this->meta_, sizeof(shm_meta_t));
}

shmhdl::shmhdl(std::string_view name, const shmsz_t nbytes,
               std::error_code &ec) noexcept{
                 
  ec.clear();
  // create a shared memory object
  int __fd = shm_open(name.data(), (int)O_FLAGS::CREATE_ONLY, (int)PERM::ALL);
  if (__fd == -1) {
    ec.assign(errno, std::system_category());
    return;
  }
  // setup shared memory object size
  if (ftruncate(__fd, nbytes + sizeof(shm_meta_t)) == -1) {
    ec.assign(errno, std::system_category());
    shm_unlink(name.data());
    return;
  }
  // map
  char *pMetaBuf = static_cast<char *>(mmap(nullptr, sizeof(shm_meta_t),
                                            PROT_EXEC | PROT_READ | PROT_WRITE,
                                            MAP_SHARED, __fd, 0));

  // fail to map
  if (pMetaBuf == (void *)-1) {
    ec.assign(errno, std::system_category());
    close(__fd);
    shm_unlink(name.data());
    return;
  }

  // success
  this->meta_ = new (pMetaBuf) shm_meta_t;
  this->meta_->ref_count_ = 1;
  this->meta_->shmsz_ = nbytes;
  this->meta_->status_ = SHM_STATUS::OK;

  this->fd_ = __fd;
  this->name_ = {name.begin(), name.end()};
  this->addr_ = nullptr;
}

shmhdl::shmhdl(std::string_view name, const shmsz_t nbytes) {
  std::error_code ec;

  int __fd = shm_open(name.data(), (int)O_FLAGS::CREATE_ONLY, (int)PERM::ALL);
  if (__fd == -1) {
    ec.assign(errno, std::system_category());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  // setup shared memory object size
  if (ftruncate(__fd, nbytes + sizeof(shm_meta_t)) == -1) {
    ec.assign(errno, std::system_category());
    shm_unlink(name.data());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  // map
  char *pMetaBuf = static_cast<char *>(mmap(nullptr, sizeof(shm_meta_t),
                                            PROT_EXEC | PROT_READ | PROT_WRITE,
                                            MAP_SHARED, __fd, 0));

  // fail to map
  if (pMetaBuf == (void *)-1) {
    ec.assign(errno, std::system_category());
    close(__fd);
    shm_unlink(name.data());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }

  // success
  this->meta_ = new (pMetaBuf) shm_meta_t;
  this->meta_->ref_count_ = 1;
  this->meta_->shmsz_ = nbytes;
  this->meta_->status_ = SHM_STATUS::OK;

  this->fd_ = __fd;
  this->name_ = {name.begin(), name.end()};
  this->addr_ = nullptr;
}

shmhdl::shmhdl(std::string_view name, std::error_code &ec) noexcept {
  ec.clear();
  int __fd = shm_open(name.data(), static_cast<int>(O_FLAGS::OPEN_ONLY),
                      static_cast<int>(PERM::ALL));
  // fail to open
  if (__fd == -1) {
    ec.assign(errno, std::system_category());
    return;
  }
  void *pMetaBuf =
      mmap(nullptr, sizeof(shm_meta_t), PROT_EXEC | PROT_READ | PROT_WRITE,
           MAP_SHARED, __fd, 0);

  // fail to map
  if (pMetaBuf == (void *)-1) {
    ec.assign(errno, std::system_category());
    close(__fd);
    shm_unlink(name.data());
    return;
  }
  // success
  this->meta_ = reinterpret_cast<shm_meta_t *>(pMetaBuf);
  this->meta_->ref_count_ += 1;

  this->fd_ = __fd;
  this->name_ = {name.begin(), name.end()};
  this->addr_ = nullptr;
}

shmhdl::shmhdl(std::string_view name) {
  std::error_code ec;

  int __fd = shm_open(name.data(), static_cast<int>(O_FLAGS::OPEN_ONLY),
                      static_cast<int>(PERM::ALL));
  // fail to open
  if (__fd == -1) {
    ec.assign(errno, std::system_category());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  void *pMetaBuf =
      mmap(nullptr, sizeof(shm_meta_t), PROT_EXEC | PROT_READ | PROT_WRITE,
           MAP_SHARED, __fd, 0);

  // fail to map
  if (pMetaBuf == (void *)-1) {
    ec.assign(errno, std::system_category());
    close(__fd);
    shm_unlink(name.data());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  // success
  this->meta_ = reinterpret_cast<shm_meta_t *>(pMetaBuf);
  this->meta_->ref_count_ += 1;

  this->fd_ = __fd;
  this->name_ = {name.begin(), name.end()};
  this->addr_ = nullptr;
}

shmhdl::~shmhdl() {
  std::error_code ec;
  if (this->fd() > 0) {
    this->meta_->ref_count_ -= 1;
    if (this->meta_->ref_count_ == 0) {
      this->meta_->status_ = SHM_STATUS::DEL;
      this->unmap(ec);
      this->unmap_meta(ec);
      close(fd_);
      fd_ = -1;
      shm_unlink(name_.c_str());
      return;
    }
    this->unmap(ec);
    this->unmap_meta(ec);
    close(fd_);
    fd_ = -1;
  }
}

void *shmhdl::map(std::error_code &ec) noexcept {
  // if already map, return address
  if (this->addr_) {
    return this->addr_;
  }

  // if haven't map
  void *__tptr = mmap(nullptr, this->meta_->shmsz_,
                      PROT_EXEC | PROT_WRITE | PROT_READ, MAP_SHARED, fd_, 0);
  if (__tptr == (void *)-1) {
    ec.assign(errno, std::system_category());
    return nullptr;
  }
  this->addr_ = reinterpret_cast<char *>(__tptr) + sizeof(shm_meta_t);
  return this->addr_;
}

void *shmhdl::map() {
  std::error_code ec;
  void *__addr = this->map(ec);
  if (ec) {
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  return __addr;
}

void shmhdl::unmap(std::error_code &ec) noexcept {
  ec.clear();
  // if addr is not nullptr
  if (this->addr_) {
    int rv = munmap(static_cast<char *>(addr_) - sizeof(shm_meta_t),
                    this->meta_->shmsz_ + sizeof(shm_meta_t));
    if (rv == -1) {
      ec.assign(errno, std::system_category());
      return;
    }
    this->addr_ = nullptr;
  }
}

void shmhdl::unmap() {
  std::error_code ec;
  this->unmap(ec);
  if (ec) {
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
}

void shmhdl::unlink(std::error_code &ec) noexcept {
  ec.clear();
  if (this->fd_ != -1) {
    int rv = shm_unlink(name_.c_str());
    this->fd_ = -1;
    if (rv == -1) {
      ec.assign(errno, std::system_category());
      return;
    }
    this->meta_->status_ = SHM_STATUS::DEL;
  }
}
void shmhdl::unlink() {
  std::error_code ec;
  this->unlink(ec);
  if (ec) {
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
}

shmsz_t shmhdl::nbytes() const noexcept { return this->meta_->shmsz_; }

int shmhdl::fd() const noexcept { return this->fd_; }

std::string_view shmhdl::name() const noexcept { return this->name_; }

void *shmhdl::addr() const noexcept { return this->addr_; }

size_t shmhdl::ref_count() const noexcept { return this->meta_->ref_count_; }

} // namespace ipc
