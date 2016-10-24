//
// Copyright (c) 2016 okertanov@gmail.com
//

#include <memory>
#include <cstdlib>
#include <iostream>
#include "vm.h"
#include "util.h"

static void usage(std::string name);

//
// Simple OSX Hypervisor
// See: https://developer.apple.com/reference/hypervisor
//
int main(int argc, char const *argv[]) {
    std::atexit(vm::util::exit_handler);

    if (argc <= 1) {
        usage(argv[0]);
        std::exit(EXIT_SUCCESS);
    }

    try {
        std::string vm_path = std::string(argv[1]);
        std::cout << "Starting VM '" << vm_path << "'" << std::endl;

        std::unique_ptr<vm::VirtualMachine> vm = std::make_unique<vm::VirtualMachine>();
        vm->Load(argv[1]);

        vm->Run([]() -> bool {
            return true;
        });
    }
    catch(std::exception& e) {
        std::cerr << "!!!!! Exception: " << e.what() << std::endl;
    }

    std::cout << "Done." << std::endl;
}

static void usage(std::string name) {
    std:: cout
            << "Usage: "
            << std::endl
            << "\t"
            << name
            << " "
            << "<path to image file>"
            << std::endl;
}
