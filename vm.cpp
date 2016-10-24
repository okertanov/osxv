//
// Copyright (c) 2016 okertanov@gmail.com
//

#include <memory>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "cstdlib"
#include "vm.h"
#include "util.h"

namespace vm {
    VirtualMachine::VirtualMachine() : _vcpu(), _vmem() {
        _vcpu = new hv_vcpuid_t(0L);

        DEBUG("Creating Virtual Machine.");

        void *_vmem = valloc(MemSize);
        VCHECK(_vmem != nullptr, "Failed to allocate virtual memory.");
        DEBUG("Memory allocated: (" << (MemSize / 1024 / 1024) << "Mb) -> " << _vmem);

        std::memset(_vmem, 0x90, MemSize - 1);
        DEBUG("Memory initialized.");

        hv_return_t status = hv_vm_create(HV_VM_DEFAULT);
        VFAILED(status, "Failed to create VM.");
        DEBUG("VM created.");

        status = hv_vm_map(_vmem, 0, MemSize, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
        VFAILED(status, "Failed to map virtual memory.");
        DEBUG("Memory mapped.");

        status = hv_vcpu_create(const_cast<hv_vcpuid_t*>(_vcpu), HV_VCPU_DEFAULT);
        VFAILED(status, "Failed to initialize vCPU.");
        DEBUG("vCPU created: " << _vcpu);

        InitVM();
    }

    VirtualMachine::~VirtualMachine() {
        hv_return_t status = hv_vcpu_destroy(*_vcpu);
        VFAILED(status, "Failed to destroy vCPU.");

        status = hv_vm_unmap(0, MemSize);
        VFAILED(status, "Failed to unmap virtual memory.");

        status = hv_vm_destroy();
        VFAILED(status, "Failed to destroy VM.");

        std::free(_vmem);
        _vmem = nullptr;

        delete _vcpu;
        _vcpu = nullptr;

        DEBUG("Virtual Machine destroyed.");
    }

    void VirtualMachine::Load(const std::string& path) {
        UNUSED(path);
    }

    void VirtualMachine::Run(const std::function<bool(void)>& tick_cb) {
        bool cont = true;

        do {
            DEBUG("Run");
            hv_return_t status = hv_vcpu_run(*_vcpu);
            VFAILED(status, "VM Runtime error: " << (((int32_t)status) & 0x000000FF));

            uint64_t exit_reason = ReadVmcs(VMCS_RO_EXIT_REASON) & 0xFFFF;
            DEBUG("Running: " << exit_reason);

            cont = DispatchVMExit(exit_reason);
            VCHECK(cont == true, "DispatchVMExit stopped.");

            cont = cont && tick_cb();
            VCHECK(cont == true, "Tick stopped.");
        } while (cont);
    }

    void VirtualMachine::InitVM() {
        //
        // Capabilities
        //
        uint64_t vmx_cap_pinbased = ReadCapability(HV_VMX_CAP_PINBASED);
        uint64_t vmx_cap_procbased = ReadCapability(HV_VMX_CAP_PROCBASED);
        uint64_t vmx_cap_procbased2 = ReadCapability(HV_VMX_CAP_PROCBASED2);
        uint64_t vmx_cap_entry = ReadCapability(HV_VMX_CAP_ENTRY);
        uint64_t vmx_cap_exit = ReadCapability(HV_VMX_CAP_EXIT);
        uint64_t vmx_cap_preemp_timer = ReadCapability(HV_VMX_CAP_PREEMPTION_TIMER);
        UNUSED(vmx_cap_exit), UNUSED(vmx_cap_preemp_timer);

        //
        // vCPU Setup
        //
        const uint64_t VMCS_PRI_PROC_BASED_CTLS_HLT = (1 << 7);
        const uint64_t VMCS_PRI_PROC_BASED_CTLS_CR8_LOAD = (1 << 19);
        const uint64_t VMCS_PRI_PROC_BASED_CTLS_CR8_STORE = (1 << 20);

	    //
        // Set VMCS control fields
        //
        WriteVmcs(VMCS_CTRL_PIN_BASED, CapabilityToControl(vmx_cap_pinbased, 0));
        WriteVmcs(VMCS_CTRL_PIN_BASED, CapabilityToControl(vmx_cap_procbased,
                                                   VMCS_PRI_PROC_BASED_CTLS_HLT |
                                                   VMCS_PRI_PROC_BASED_CTLS_CR8_LOAD |
                                                   VMCS_PRI_PROC_BASED_CTLS_CR8_STORE));
	    WriteVmcs(VMCS_CTRL_CPU_BASED2, CapabilityToControl(vmx_cap_procbased2, 0));
	    WriteVmcs(VMCS_CTRL_VMENTRY_CONTROLS, CapabilityToControl(vmx_cap_entry, 0));
    	WriteVmcs(VMCS_CTRL_EXC_BITMAP, 0xFFFFFFFF);
    	WriteVmcs(VMCS_CTRL_CR0_MASK, 0x60000000);
    	WriteVmcs(VMCS_CTRL_CR0_SHADOW, 0);
    	WriteVmcs(VMCS_CTRL_CR4_MASK, 0);
    	WriteVmcs(VMCS_CTRL_CR4_SHADOW, 0);

    	//
        // Set VMCS guest state fields
        //
    	WriteVmcs(VMCS_GUEST_CS, 0);
    	WriteVmcs(VMCS_GUEST_CS_LIMIT, 0xffff);
    	WriteVmcs(VMCS_GUEST_CS_AR, 0x9b);
    	WriteVmcs(VMCS_GUEST_CS_BASE, 0);

    	WriteVmcs(VMCS_GUEST_DS, 0);
    	WriteVmcs(VMCS_GUEST_DS_LIMIT, 0xffff);
    	WriteVmcs(VMCS_GUEST_DS_AR, 0x93);
    	WriteVmcs(VMCS_GUEST_DS_BASE, 0);

    	WriteVmcs(VMCS_GUEST_ES, 0);
    	WriteVmcs(VMCS_GUEST_ES_LIMIT, 0xffff);
    	WriteVmcs(VMCS_GUEST_FS_AR, 0x93);
    	WriteVmcs(VMCS_GUEST_ES_BASE, 0);

    	WriteVmcs(VMCS_GUEST_FS, 0);
    	WriteVmcs(VMCS_GUEST_FS_LIMIT, 0xffff);
    	WriteVmcs(VMCS_GUEST_FS_AR, 0x93);
    	WriteVmcs(VMCS_GUEST_FS_BASE, 0);

    	WriteVmcs(VMCS_GUEST_GS, 0);
    	WriteVmcs(VMCS_GUEST_GS_LIMIT, 0xffff);
    	WriteVmcs(VMCS_GUEST_GS_AR, 0x93);
    	WriteVmcs(VMCS_GUEST_GS_BASE, 0);

    	WriteVmcs(VMCS_GUEST_SS, 0);
    	WriteVmcs(VMCS_GUEST_SS_LIMIT, 0xffff);
    	WriteVmcs(VMCS_GUEST_SS_AR, 0x93);
    	WriteVmcs(VMCS_GUEST_SS_BASE, 0);

    	WriteVmcs(VMCS_GUEST_LDTR, 0);
    	WriteVmcs(VMCS_GUEST_LDTR_LIMIT, 0);
    	WriteVmcs(VMCS_GUEST_LDTR_AR, 0x10000);
    	WriteVmcs(VMCS_GUEST_LDTR_BASE, 0);

    	WriteVmcs(VMCS_GUEST_TR, 0);
    	WriteVmcs(VMCS_GUEST_TR_LIMIT, 0);
    	WriteVmcs(VMCS_GUEST_TR_AR, 0x83);
    	WriteVmcs(VMCS_GUEST_TR_BASE, 0);

    	WriteVmcs(VMCS_GUEST_GDTR_LIMIT, 0);
    	WriteVmcs(VMCS_GUEST_GDTR_BASE, 0);

    	WriteVmcs(VMCS_GUEST_IDTR_LIMIT, 0);
    	WriteVmcs(VMCS_GUEST_IDTR_BASE, 0);

    	WriteVmcs(VMCS_GUEST_CR0, 0x20);
    	WriteVmcs(VMCS_GUEST_CR3, 0x0);
    	WriteVmcs(VMCS_GUEST_CR4, 0x2000);

    	//
        // Set up GPRs
        //
    	WriteReg(HV_X86_RIP, 0x100);
    	WriteReg(HV_X86_RFLAGS, 0x2);
    	WriteReg(HV_X86_RSP, 0x00);
    }

    bool VirtualMachine::DispatchVMExit(uint64_t reason) {
        bool cont = true;

        uint64_t rip = ReadReg(HV_X86_RIP);
        DEBUG("RIP: " << rip);

        switch (reason) {
            case VMX_REASON_IRQ: {
                uint32_t soft_irq= ReadVmcs(IRQ_INFO_SOFT_IRQ);
                DEBUG("Dispatch VM Exit: " << "IRQ: " << soft_irq);
            }
            break;
            case VMX_REASON_HLT:
                DEBUG("Dispatch VM Exit: " << "HLT");
                cont = false;
            break;
            case VMX_REASON_EPT_VIOLATION:
                DEBUG("Dispatch VM Exit: " << "EPT VIOLATION");
                cont = false;
            break;
            case VMX_REASON_TRIPLE_FAULT:
                DEBUG("Dispatch VM Exit: " << "TRIPLE FAULT");
                cont = false;
            break;
            case VMX_REASON_VMENTRY_GUEST:
                DEBUG("Dispatch VM Exit: " << "VMENTRY GUEST");
            break;
            case VMX_REASON_IO:
                DEBUG("Dispatch VM Exit: " << "IO");
            break;
            default:
                DEBUG("Dispatch VM Exit: " << "UNKNOWN: " << reason);
            break;
        }

        size_t inst_length = ReadVmcs(VMCS_RO_VMEXIT_INSTR_LEN);
        size_t next_rip = rip + inst_length;
        WriteReg(HV_X86_RIP, next_rip);

        return cont;
    }

    uint64_t VirtualMachine::ReadReg(hv_x86_reg_t reg) {
        uint64_t val = 0L;
        hv_return_t status = hv_vcpu_read_register(*_vcpu, reg, &val);
        VFAILED(status, "Failed to read CPU register.");
        return val;
    }

    void VirtualMachine::WriteReg(hv_x86_reg_t reg, uint64_t val) {
        hv_return_t status = hv_vcpu_write_register(*_vcpu, reg, val);
        VFAILED(status, "Failed to write CPU register.");
    }

    uint64_t VirtualMachine::ReadVmcs(uint32_t field) {
        uint64_t val= 0L;
        hv_return_t status = hv_vmx_vcpu_read_vmcs(*_vcpu, field, &val);
        VFAILED(status, "Failed to read VM Control structure.");
        return val;
    }

    void VirtualMachine::WriteVmcs(uint32_t field, uint64_t val) {
        hv_return_t status = hv_vmx_vcpu_write_vmcs(*_vcpu, field, val);
        VFAILED(status, "Failed to write VM Control structure.");
    }

    uint64_t VirtualMachine::ReadCapability(hv_vmx_capability_t cap) {
        uint64_t cval = 0L;
        hv_return_t status = hv_vmx_read_capability(cap, &cval);
        VFAILED(status, "Failed to read VM Capability.");
        return cval;
    }

    uint64_t VirtualMachine::CapabilityToControl(uint64_t cap, uint64_t ctrl) {
        uint64_t ca2co = (ctrl | (cap & 0xFFFFFFFF)) & (cap >> 32);
        return ca2co;
    }
}
