#include "semhdl.hpp"
#include <catch2/catch.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("create semhdl with given name and value", "[create]") {
  std::error_code ec;
  ipc::semhdl hdl("test", 0, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(hdl.name().compare("test") == 0);
  int val = hdl.value(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(val == 0);
}

TEST_CASE("create semhdl with same name", "[create]") {
  std::error_code ec;
  ipc::semhdl hdl1("test", 0, ec);
  REQUIRE_FALSE(ec);
  REQUIRE(hdl1.name().compare("test") == 0);
  int val = hdl1.value(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(val == 0);

  ipc::semhdl hdl2("test", 0, ec);
  REQUIRE(ec);
}

TEST_CASE("wait/post in same process", "[wait]") {
  std::error_code ec;
  ipc::semhdl hdl("test", 0, ec);
  REQUIRE_FALSE(ec);

  std::thread t;
  t = std::thread([&hdl, &ec]() {
    std::this_thread::sleep_for(200ms);
    int val = hdl.value(ec);
    REQUIRE_FALSE(ec);
    REQUIRE(val == 0);
    hdl.post(ec);
    REQUIRE_FALSE(ec);
    val = hdl.value(ec);
    REQUIRE_FALSE(ec);
    REQUIRE(val == 1);
  });
  hdl.wait(ec);
  REQUIRE_FALSE(ec);

  int val = hdl.value(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(val == 0);
  t.join();
}

TEST_CASE("wait/post in different process", "[wait]") {
  std::error_code ec;
  int val;
  ipc::semhdl hdl1("test", 0, ec);
  REQUIRE_FALSE(ec);
  val = hdl1.value(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(val == 0);

  ipc::semhdl hdl2("test", ec);
  REQUIRE_FALSE(ec);
  val = hdl2.value(ec);
  REQUIRE_FALSE(ec);
  REQUIRE(val == 0);

  std::thread t([&ec, &hdl2, &hdl1]() {
    std::this_thread::sleep_for(200ms);
    int val1, val2;

    hdl2.post(ec);
    REQUIRE_FALSE(ec);
    val1 = hdl1.value(ec);
    REQUIRE_FALSE(ec);
    val2 = hdl2.value(ec);
    REQUIRE_FALSE(ec);
    REQUIRE(val1 == 1);
    REQUIRE(val2 == 1);
  });

  hdl1.wait(ec);
  t.join();
  REQUIRE_FALSE(ec);
  val = hdl1.value(ec);
  REQUIRE(val == 0);
  val = hdl2.value(ec);
  REQUIRE(val == 0);
}