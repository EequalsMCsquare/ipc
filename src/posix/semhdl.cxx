#include "semhdl.hpp"

#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <semaphore.h>
#include <stdexcept>
#include <system_error>

namespace ipc {
semhdl::semhdl(std::string_view name, const uint32_t value,
               std::error_code &ec) noexcept {
  ec.clear();
  auto __sema = sem_open(name.data(), static_cast<int>(O_FLAGS::CREATE_ONLY),
                         0644, value);
  // fail
  if (__sema == nullptr) {
    ec.assign(errno, std::system_category());
    sem_unlink(name.data());
    return;
  }
  // success
  this->sema_ = __sema;
  this->name_ = {name.begin(), name.end()};
}
semhdl::semhdl(std::string_view name, const uint32_t value) {
  std::error_code ec;
  auto __sema = sem_open(name.data(), static_cast<int>(O_FLAGS::CREATE_ONLY),
                         0644, value);
  // fail
  if (__sema == nullptr) {
    ec.assign(errno, std::system_category());
    sem_unlink(name.data());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  // success
  this->sema_ = __sema;
  this->name_ = {name.begin(), name.end()};
}

semhdl::semhdl(std::string_view name, std::error_code &ec) noexcept {
  ec.clear();
  auto __sem = sem_open(name.data(), static_cast<int>(O_FLAGS::OPEN_ONLY));

  if (__sem == nullptr) {
    ec.assign(errno, std::system_category());
    return;
  }
  // success
  this->sema_ = __sem;
  this->name_ = {name.begin(), name.end()};
}

semhdl::semhdl(std::string_view name) {
  std::error_code ec;
  auto __sem = sem_open(name.data(), static_cast<int>(O_FLAGS::OPEN_ONLY));

  if (__sem == nullptr) {
    ec.assign(errno, std::system_category());
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
    return;
  }
  // success
  this->sema_ = __sem;
  this->name_ = {name.begin(), name.end()};
}

semhdl::~semhdl() {
  sem_close(this->sema_);
  if (sem_unlink(this->name_.data()) == -1) {
    std::error_code ec(errno, std::system_category());
    std::cerr << ec.message() << std::endl;
  }
}

void semhdl::wait(std::error_code &ec) noexcept {
  ec.clear();
  if (sem_wait(this->sema_) == -1) {
    ec.assign(errno, std::system_category());
  }
}

void semhdl::wait() {
  std::error_code ec;
  this->wait(ec);
  if (ec) {
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
}

void semhdl::post(std::error_code &ec) noexcept {
  ec.clear();
  if (sem_post(this->sema_) == -1) {
    ec.assign(errno, std::system_category());
  }
}

void semhdl::post() {
  std::error_code ec;
  this->post(ec);
  if (ec) {
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
}

void semhdl::try_wait(std::error_code &ec) noexcept {
  ec.clear();
  if (sem_trywait(this->sema_) == -1) {
    ec.assign(errno, std::system_category());
  }
}

int semhdl::value(std::error_code &ec) const noexcept {
  ec.clear();
  int val;
  if (sem_getvalue(this->sema_, &val) == -1) {
    ec.assign(errno, std::system_category());
  }
  return val;
}

int semhdl::value() const {
  std::error_code ec;
  int val = this->value(ec);
  if (ec) {
    char errmsg[256];
    snprintf(errmsg, 256, "(%d) %s", ec.value(), ec.message().data());
    throw std::runtime_error(errmsg);
  }
  return val;
}

std::string_view semhdl::name() const noexcept { return this->name_; }
} // namespace ipc
