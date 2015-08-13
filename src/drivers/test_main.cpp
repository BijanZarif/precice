#include <boost/test/unit_test.hpp>
#include <iostream>

struct GlobalFixture {
  GlobalFixture()
  {
    std::cout << "Global setup\n";
    // boost::unit_test::master_test_suite_t& master = boost::unit_test::framework::master_test_suite();
    // BOOST_FAIL("CRASH BOOM"); Use exceptions if you want to fail inside a 
  }
  ~GlobalFixture()
  {
    std::cout << "Global teardown\n";
  }
};

BOOST_GLOBAL_FIXTURE(GlobalFixture)
