//
// Copyright (c) 2016 okertanov@gmail.com
//

#pragma once

#define UNUSED(X) (void)(X)

#define VCHECK(E, M) do { if (!(E)) { std::stringstream ss; ss << M; throw std::runtime_error(ss.str()); } } while (false)
#define VFAILED(S, M) VCHECK(S == HV_SUCCESS, M)

#define DEBUG(S) do { std::cout << ">>>>> DEBUG: " << S << std::endl; } while (false)

namespace vm {
    namespace util {
        void exit_handler(void);
    }
}
