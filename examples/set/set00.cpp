#include <iostream>
#include <set>

typedef std::set<int32_t> TestSet;

int main() {
  TestSet set;

  set.insert(1);
  set.insert(2);
  set.insert(3);
  set.insert(4);
  set.insert(1);
  set.insert(5);

  for (int32_t num : set) {
    std::cout << "(" << num << "), ";
  }

  std::cout << std::endl;
}
