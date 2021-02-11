#include "shmhdl.hpp"
#include <array>
#include <catch2/catch.hpp>
#include <memory>
#include <random>

TEST_CASE("create shmhdl with given name and nbytes", "[create]") {
  std::error_code ec;
  ipc::shmhdl hdl("test", 4096, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(hdl.addr() == nullptr);
  REQUIRE(hdl.name().compare("test") == 0);
  REQUIRE(hdl.nbytes() >= 4096);
  REQUIRE(hdl.ref_count() == 1);
}

TEST_CASE("create shmhdl with same name", "[create]") {
  std::error_code ec;
  ipc::shmhdl hdl1("test", 4096, ec);
  REQUIRE_FALSE(ec);
  ipc::shmhdl hdl2("test", 4096, ec);
  REQUIRE(ec);
}

TEST_CASE("create shmhdl and destroy it", "[create]") {
  std::error_code ec;
  {
    ipc::shmhdl hdl("test", 4096, ec);
    REQUIRE_FALSE(ec);
  }
  {
    ipc::shmhdl hdl("test", 1024, ec);
    REQUIRE_FALSE(ec);
  }
  {
    ipc::shmhdl hdl("test", 1024 * 1024 * 1024, ec);
    REQUIRE_FALSE(ec);
  }
}

TEST_CASE("attach to existing shmhdl 1", "[create]") {
  std::error_code ec;
  ipc::shmhdl svr("test", 4096, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr.ref_count() == 1);

  ipc::shmhdl clt1("test", ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr.ref_count() == 2);

  ipc::shmhdl clt2("test", ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr.ref_count() == 3);
}

TEST_CASE("attach to existing shmhdl 2", "[create]") {
  std::error_code ec;
  std::vector<std::unique_ptr<ipc::shmhdl>> clts;
  clts.reserve(10);
  // server
  ipc::shmhdl svr("test", 4096, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr.ref_count() == 1);

  // clients
  for (int i = 0; i < 10; i++) {
    clts.emplace_back(std::make_unique<ipc::shmhdl>("test", ec));
    REQUIRE_FALSE(ec);
  }
  REQUIRE(svr.ref_count() == 11);
  clts.clear();
  REQUIRE(svr.ref_count() == 1);
}

TEST_CASE("attach to existing shmhdl 3", "[create]") {
  std::error_code ec;
  std::vector<std::shared_ptr<ipc::shmhdl>> clts;
  clts.reserve(10);
  // server
  ipc::shmhdl svr("test", 4096, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr.ref_count() == 1);

  for (int i = 0; i < 20; i++) {
    clts.emplace_back(std::make_shared<ipc::shmhdl>("test", ec));
    auto __crthdl = clts[i];
    REQUIRE(__crthdl->ref_count() == i + 2);
    REQUIRE(__crthdl->ref_count() == svr.ref_count());
    REQUIRE(__crthdl->name().compare(svr.name()) == 0);
    REQUIRE_FALSE(ec);
  }
  REQUIRE(svr.ref_count() == 21);
  for (int i = 1; i <= 20; i++) {
    clts.erase(clts.begin());
    REQUIRE(clts.size() == 20 - i);
    REQUIRE(svr.ref_count() == 21 - i);
    for (const auto e : clts) {
      REQUIRE(e->ref_count() == 21 - i);
    }
  }

  REQUIRE(clts.size() == 0);
  REQUIRE(svr.ref_count() == 1);
}

TEST_CASE("map/unmap shmhdl into current process 1", "[map]") {
  std::error_code ec;
  ipc::shmhdl hdl("test", 4096, ec);
  REQUIRE_FALSE(ec);

  void *__addr = hdl.map(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(__addr != nullptr);
  REQUIRE(hdl.addr() == __addr);

  hdl.unmap(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(hdl.addr() == nullptr);
}

TEST_CASE("map/unmap shmhdl into current process 2", "[map]") {
  std::error_code ec;
  ipc::shmhdl hdl("test", 4096, ec);
  REQUIRE_FALSE(ec);

  std::vector<void *> __addrs;
  __addrs.reserve(10);

  for (int i = 0; i < 10; i++) {
    void *__tmp = hdl.map(ec);
    REQUIRE_FALSE(ec);
    REQUIRE(__tmp != nullptr);
    __addrs.emplace_back(__tmp);
  }
  REQUIRE(__addrs.size());
  __addrs.clear();
  REQUIRE(__addrs.size() == 0);
}

TEST_CASE("map/unmap shmhdl into current process 3", "[map]") {
  std::error_code ec;
  ipc::shmhdl hdl("test", 1024, ec);
  REQUIRE_FALSE(ec);

  void *__addr1 = hdl.map(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(__addr1 != nullptr);

  hdl.unmap(ec);
  REQUIRE_FALSE(ec);

  void *__addr2 = hdl.map(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(__addr2 != nullptr);
  CHECK(__addr2 == __addr1);
}

TEST_CASE("check data stored in shmhdl 1", "[data]") {
  std::error_code ec;
  constexpr size_t __nbytes = 1024;
  auto svr = new ipc::shmhdl("test", __nbytes, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr->ref_count() == 1);
  ipc::shmhdl clt("test", ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr->ref_count() == 2);
  REQUIRE(clt.ref_count() == 2);

  double *svr_ptr = static_cast<double *>(svr->map(ec));
  REQUIRE_FALSE(ec);
  REQUIRE(svr_ptr != nullptr);

  // store random number into the shmhdl
  std::mt19937 engine;
  std::uniform_real_distribution<double> generator(0.0, 1.0);
  size_t len = __nbytes / 8;
  std::vector<double> arr;
  arr.reserve(len);
  size_t i;

  double __tmp;
  for (i = 0; i < len; i++) {
    __tmp = generator.operator()(engine);
    svr_ptr[i] = __tmp;
    arr.emplace_back(__tmp);
  }

  delete svr;

  double* clt_ptr = static_cast<double*>(clt.map(ec));
  REQUIRE_FALSE(ec);
  for(i = 0; i < len; i++) {
    REQUIRE(clt_ptr[i] == arr[i]);
  }
}

TEST_CASE("check data stored in shmhdl 2", "[data]") {
  std::error_code ec;
  constexpr size_t __nbytes = 1024;
  auto svr = new ipc::shmhdl("test", __nbytes, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr->ref_count() == 1);
  ipc::shmhdl clt("test", ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr->ref_count() == 2);
  REQUIRE(clt.ref_count() == 2);

  double *svr_ptr = static_cast<double *>(svr->map(ec));
  REQUIRE_FALSE(ec);
  REQUIRE(svr_ptr != nullptr);

  // store random number into the shmhdl
  std::mt19937 engine;
  std::uniform_real_distribution<double> generator(0.0, 1.0);
  size_t len = __nbytes / 8;
  size_t i;
  size_t svr_sum = 0;
  size_t clt_sum = 0;

  double __tmp;
  for (i = 0; i < len; i++) {
    __tmp = generator.operator()(engine);
    svr_ptr[i] = __tmp;
    svr_sum += __tmp;
  }
  // delete svr
  delete svr;

  // setup client shmhdl
  double *clt_ptr = static_cast<double *>(clt.map(ec));
  REQUIRE_FALSE(ec);
  REQUIRE(clt_ptr);

  for (i = 0; i < len; i++) {
    clt_sum += clt_ptr[i];
  }
  REQUIRE(svr_sum == Approx(clt_sum));
}

TEST_CASE("unlink will privent other shmhdl to attach to it", "[unlink]") {
  std::error_code ec;
  ipc::shmhdl svr("test", 4096, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(svr.ref_count() == 1);

  ipc::shmhdl clt1("test", ec);
  REQUIRE_FALSE(ec);
  REQUIRE(clt1.ref_count() == 2);
  REQUIRE(svr.ref_count() == 2);

  svr.unlink(ec);
  REQUIRE_FALSE(ec);

  ipc::shmhdl clt2("test", ec);
  REQUIRE(ec);
  REQUIRE(clt1.ref_count() == 2);
  REQUIRE(svr.ref_count() == 2);
}