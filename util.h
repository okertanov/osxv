//
// Copyright (c) 2016 okertanov@gmail.com
//

#pragma once

#define UNUSED(X) (void)(X)

#define VCHECK(E, M) do { if (!(E)) { std::stringstream ss; ss << M; throw std::runtime_error(ss.str()); } } while (false)
#define VFAILED(S, M) VCHECK(S == HV_SUCCESS, M)

#define DEBUG(S) do { std::cout << ">>>>> DEBUG: " << S << std::endl; } while (false)

#define OUT_INLINE(S) do { std::cout << "\033[?25l" << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" << "\033[32m" << S << "\033[39m" << "\033[?25h"; } while (false)

namespace vm {
    namespace util {
        void exit_handler(void);
    }
}
