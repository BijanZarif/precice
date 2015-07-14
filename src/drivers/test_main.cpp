#include <boost/test/unit_test.hpp>
#include <iostream>

struct GlobalFixture {
  GlobalFixture()
  {
    std::cout << "Global setup\n";
  }
  ~GlobalFixture()
  {
    std::cout << "Global teardown\n";
  }
};

BOOST_GLOBAL_FIXTURE(GlobalFixture)
