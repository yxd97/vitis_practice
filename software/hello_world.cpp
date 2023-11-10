#include <iostream>
#include <dlfcn.h>
#include <stdio.h>

#include "above.hpp"
#include "ampersand.hpp"


int main(int argc, char** argv) {
    void* handle = dlopen("./libbeyond.so", RTLD_NOW);
    if (!handle) {
        std::cerr << "Cannot open library: " << dlerror() << '\n';
        return 1;
    }
    void* fptr = dlsym(handle, "beyond");
    void (*beyond)() = (void (*)()) fptr;

    std::cout << "Hello World!" << std::endl;
    above();
    std::cout << " ";
    ampersand();
    std::cout << " ";
    beyond();
    std::cout << "\n";
    dlclose(handle);
    return 0;
}
