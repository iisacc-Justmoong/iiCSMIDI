#include <iiCSMIDI.h>
#include <iostream>
static_assert(__cplusplus>202002L, "A C++23 language mode is required");
int main() {
    const auto message=iiCSMIDI::helloWorld(); std::cout<<message<<'\n';
    return message=="Hello world!" ? 0 : 1;
}
