//
// Copyright (c) 2016 okertanov@gmail.com
//

#pragma once

#include <string>
#include <functional>
#include <Hypervisor/hv.h>
#include <Hypervisor/hv_vmx.h>

namespace vm {
    class VirtualMachine {
        public:
            VirtualMachine();
            ~VirtualMachine();
            void Load(const std::string& path);
            void Run(const std::function<bool(void)>& tick_cb);
        private:
            static const size_t MemSize = 0x40000000;
            const hv_vcpuid_t* _vcpu;
            void* _vmem;

            void InitVM();

            bool DispatchVMExit(uint64_t reason);

            uint64_t ReadReg(hv_x86_reg_t reg);
            void WriteReg(hv_x86_reg_t reg, uint64_t val);

            uint64_t ReadVmcs(uint32_t field);
            void WriteVmcs(uint32_t field, uint64_t val);

            uint64_t ReadCapability(hv_vmx_capability_t cap);
            uint64_t CapabilityToControl(uint64_t cap, uint64_t ctrl);
    };
}
