#include "../src/fcmsketch.h"
#include <assert.h>
#include <typeinfo>

void smokeTest() {
  std::cout << "Running smokeTest..." << std::endl;
  assert(true == true);
  std::cout << "Finished smokeTest" << std::endl;
}

void test_construct() {
  std::cout << "Constructing test..." << std::endl;
  FCM_Sketch fcmsketch(3, 0);
  assert(typeid(fcmsketch) == typeid(FCM_Sketch));
  std::cout << "Finished construct test" << std::endl;
}
int main() {
  smokeTest();
  test_construct();
  return 0;
}
