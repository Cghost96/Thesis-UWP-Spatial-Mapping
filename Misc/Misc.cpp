#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

int main()
{
    // default constructor: empty map
    std::unordered_map<std::string, std::string> m1;

    // list constructor
    std::unordered_map<int, std::string> m2 =
    {
        {1, "foo"},
        {3, "bar"},
        {2, "baz"},
    };

    // copy constructor
    std::unordered_map<int, std::string> m3 = m2;

    // move constructor
    std::unordered_map<int, std::string> m4 = std::move(m2);
    std::unordered_map<int, std::string> m5 = std::move(m2);

    //if (m5) {
        //std::string test();
    //}

    std::cin.get();
}