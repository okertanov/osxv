//
// Copyright (c) 2016 okertanov@gmail.com
//

#include <cstdlib>
#include <iostream>
#include "util.h"

namespace vm {
    namespace util {
        void exit_handler(void) {
            std::cerr << "Exit handler: Terminating..." << std::endl;
        }
    }
}
