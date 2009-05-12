/*
 * Copyright (c) 2007 MIPS Technologies, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Korey Sewell
 *
 */

#ifndef __CPU_INORDER_TLB_UNIT_HH__
#define __CPU_INORDER_TLB_UNIT_HH__

#include <vector>
#include <list>
#include <string>

#include "cpu/inorder/resources/inst_buffer.hh"
#include "cpu/inorder/inorder_dyn_inst.hh"
#include "cpu/inorder/pipeline_traits.hh"
#include "cpu/inorder/cpu.hh"

class TLBUnit : public InstBuffer {
  public:
    typedef ThePipeline::DynInstPtr DynInstPtr;

    enum TLBCommand {
        FetchLookup,
        DataReadLookup,
        DataWriteLookup
    };

  public:
    TLBUnit(std::string res_name, int res_id, int res_width,
              int res_latency, InOrderCPU *_cpu, ThePipeline::Params *params);
    virtual ~TLBUnit() {}

    void init();

    int getSlot(DynInstPtr inst);

    virtual ResourceRequest* getRequest(DynInstPtr _inst, int stage_num,
                                        int res_idx, int slot_num,
                                        unsigned cmd);

    virtual void execute(int slot_num);

    bool tlbBlocked[ThePipeline::MaxThreads];

    TheISA::TLB* tlb();

  protected:
    /** List of instructions this resource is currently
     *  processing.
     */
    std::list<DynInstPtr> instList;

    TheISA::TLB *_tlb;
};

class TLBUnitEvent : public ResourceEvent {
  public:
    /** Constructs a resource event. */
    TLBUnitEvent();
    virtual ~TLBUnitEvent() {}

    /** Processes a resource event. */
    virtual void process();
};

class TLBUnitRequest : public ResourceRequest {
  public:
    typedef ThePipeline::DynInstPtr DynInstPtr;

  public:
    TLBUnitRequest(TLBUnit *res, DynInstPtr inst, int stage_num, int res_idx, int slot_num,
                   unsigned _cmd)
        : ResourceRequest(res, inst, stage_num, res_idx, slot_num, _cmd)
    {
        Addr aligned_addr;
        int req_size;
        unsigned flags;

        if (_cmd == TLBUnit::FetchLookup) {
            aligned_addr = inst->getMemAddr();
            req_size = sizeof(TheISA::MachInst);
            flags = 0;
        } else {
            aligned_addr = inst->getMemAddr();;
            req_size = inst->getMemAccSize();
            flags = inst->getMemFlags();
        }

        if (req_size == 0 && (inst->isDataPrefetch() || inst->isInstPrefetch())) {
            req_size = 8;
        }

        // @TODO: Add Vaddr & Paddr functions
        inst->memReq = new Request(inst->readTid(), aligned_addr, req_size,
                                   flags, inst->readPC(), res->cpu->readCpuId(), inst->readTid());

        memReq = inst->memReq;
    }

    RequestPtr memReq;
};


#endif //__CPU_INORDER_TLB_UNIT_HH__