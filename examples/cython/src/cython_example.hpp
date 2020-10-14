#include <iostream>
#include <vector>
#include <string>

#include <aff3ct.hpp>

using namespace aff3ct;

void hello(const char *name)
{
    std::cout << name << std::endl;
}

void world(int size)
{
    auto source = std::unique_ptr<module::Source_random<>>(new module::Source_random<>(size));
    std::vector<int> ref_bits(size);
    source->generate(ref_bits);
    std::for_each(ref_bits.begin(), ref_bits.end(), [&](const int i){
        std::cout << i << ' ';});
    std::cout << std::endl;
}
