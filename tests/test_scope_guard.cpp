// Unit tests for creo::ScopeGuard / creo::Defer (creo/ScopeGuard.hpp),
// using doctest (vendored: tests/doctest.h, MIT license, see its own
// header for the full notice). Each TEST_CASE below is its own
// independently reported test -- doctest supplies main() itself
// (DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN), nothing hand-rolled here.
//
// Zero ProTOOLKIT/Win32 dependency (see ScopeGuard.hpp itself), so this
// builds and runs on any platform, unlike test_property_utils.cpp next
// to it.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "creo/ScopeGuard.hpp"

#include <stdexcept>
#include <vector>

TEST_CASE("runs on normal scope exit") {
  bool ran = false;
  {
    auto guard = creo::Defer([&] { ran = true; });
    CHECK(!ran);
  }
  CHECK(ran);
}

TEST_CASE("runs when an exception unwinds the scope") {
  bool ran = false;
  bool caught = false;
  try {
    auto guard = creo::Defer([&] { ran = true; });
    throw std::runtime_error("boom");
  } catch (const std::runtime_error &) {
    caught = true;
  }
  CHECK(caught);
  CHECK(ran);
}

TEST_CASE("Dismiss() cancels the pending cleanup") {
  bool ran = false;
  {
    auto guard = creo::Defer([&] { ran = true; });
    guard.Dismiss();
  }
  CHECK(!ran);
}

TEST_CASE("Dismiss() still skips cleanup even if an exception follows") {
  bool ran = false;
  bool caught = false;
  try {
    auto guard = creo::Defer([&] { ran = true; });
    guard.Dismiss();
    throw std::runtime_error("boom");
  } catch (const std::runtime_error &) {
    caught = true;
  }
  CHECK(caught);
  CHECK(!ran);
}

TEST_CASE("moving a guard transfers ownership of the cleanup") {
  int calls = 0;
  {
    auto a = creo::Defer([&] { ++calls; });
    {
      auto b = std::move(a);
    } // b's cleanup runs here.
    CHECK(calls == 1);
  } // a, moved-from, must not run its cleanup again here.
  CHECK(calls == 1);
}

TEST_CASE("multiple guards unwind in reverse declaration order") {
  std::vector<int> order;
  {
    auto first = creo::Defer([&] { order.push_back(1); });
    auto second = creo::Defer([&] { order.push_back(2); });
    auto third = creo::Defer([&] { order.push_back(3); });
  }
  CHECK(order == std::vector<int>{3, 2, 1});
}

TEST_CASE("the capture is a snapshot at creation time") {
  int calls = 0;
  int value = 1;
  bool lambda_saw_original_value = false;
  {
    auto guard = creo::Defer([&calls, value, &lambda_saw_original_value] {
      lambda_saw_original_value = (value == 1); // captured by value
      ++calls;
    });
    value = 2; // must not affect the guard's already-captured copy
  }
  CHECK(calls == 1);
  CHECK(lambda_saw_original_value);
}
