// Unit tests for creo::ScopeGuard / creo::Defer (creo/ScopeGuard.hpp).
// Zero ProTOOLKIT dependency (see the header itself), so this runs on
// any platform/compiler, unlike test_property_utils.cpp below it. Plain
// assert()-based: no test framework dependency, matching this project's
// existing style. Exit code 0 = all tests passed.
#include "creo/ScopeGuard.hpp"

#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {

void NormalExit() {
  bool ran = false;
  {
    auto guard = creo::Defer([&] { ran = true; });
    assert(!ran);
  }
  assert(ran);
  std::puts("NormalExit: OK");
}

void ExceptionExit() {
  bool ran = false;
  bool caught = false;
  try {
    auto guard = creo::Defer([&] { ran = true; });
    throw std::runtime_error("boom");
  } catch (const std::runtime_error &) {
    caught = true;
  }
  assert(caught);
  assert(ran);
  std::puts("ExceptionExit: OK");
}

void Dismiss() {
  bool ran = false;
  {
    auto guard = creo::Defer([&] { ran = true; });
    guard.Dismiss();
  }
  assert(!ran);
  std::puts("Dismiss: OK");
}

void DismissThenExceptionStillSkipsCleanup() {
  bool ran = false;
  bool caught = false;
  try {
    auto guard = creo::Defer([&] { ran = true; });
    guard.Dismiss();
    throw std::runtime_error("boom");
  } catch (const std::runtime_error &) {
    caught = true;
  }
  assert(caught);
  assert(!ran);
  std::puts("DismissThenExceptionStillSkipsCleanup: OK");
}

void MoveTransfersOwnership() {
  int calls = 0;
  {
    auto a = creo::Defer([&] { ++calls; });
    {
      auto b = std::move(a);
    } // b's cleanup runs here.
    assert(calls == 1);
  } // a, moved-from, must NOT run its cleanup again here.
  assert(calls == 1);
  std::puts("MoveTransfersOwnership: OK");
}

void MultipleGuardsRunInReverseDeclarationOrder() {
  std::vector<int> order;
  {
    auto first = creo::Defer([&] { order.push_back(1); });
    auto second = creo::Defer([&] { order.push_back(2); });
    auto third = creo::Defer([&] { order.push_back(3); });
  }
  assert((order == std::vector<int>{3, 2, 1}));
  std::puts("MultipleGuardsRunInReverseDeclarationOrder: OK");
}

void CaptureIsSnapshotAtCreationTime() {
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
  assert(calls == 1);
  assert(lambda_saw_original_value);
  std::puts("CaptureIsSnapshotAtCreationTime: OK");
}

} // namespace

int main() {
  NormalExit();
  ExceptionExit();
  Dismiss();
  DismissThenExceptionStillSkipsCleanup();
  MoveTransfersOwnership();
  MultipleGuardsRunInReverseDeclarationOrder();
  CaptureIsSnapshotAtCreationTime();
  std::puts("OK - all ScopeGuard/Defer tests passed");
  return 0;
}
