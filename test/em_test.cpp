#include "EM_FSD.hpp"
#include "ori_EM_FCM.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
  std::cout << "Smoke done!" << std::endl;
}

TEST_CASE("Construct EMs", "[Constructor]") {
  vector<vector<vector<vector<uint32_t>>>> thresh;
  vector<vector<uint32_t>> counters;
  EM_FCM<1, 16, 3, 15> ori_em_fsd;
  ori_em_fsd.set_counters({counters}, {thresh});
  EMFSD em_fsd({3, 15}, thresh, 5, 1, counters);
}
