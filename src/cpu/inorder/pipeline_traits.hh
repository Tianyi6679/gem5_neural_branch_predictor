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

#ifndef __CPU_INORDER_PIPELINE_IMPL_HH__
#define __CPU_INORDER_PIPELINE_IMPL_HH__

#include <list>
#include <queue>
#include <vector>

#include "arch/isa_traits.hh"
#include "cpu/base.hh"
//#include "cpu/inorder/params.hh"

#include "params/InOrderCPU.hh"

class InOrderDynInst;

/* This Namespace contains constants, typedefs, functions and
 * objects specific to the Pipeline Implementation.
 */
namespace ThePipeline {
    // Pipeline Constants
    const unsigned NumStages = 5;
    const unsigned MaxThreads = 8;
    const unsigned StageWidth = 1;
    const unsigned BackEndStartStage = 2;

    // Enumerated List of Resources The Pipeline Uses
    enum ResourceList {
       FetchSeq = 0,
       ITLB,
       ICache,
       Decode,
       BPred,
       FetchBuff,
       RegManager,
       AGEN,
       ExecUnit,
       MDU,
       DTLB,
       DCache,
       Grad,
       FetchBuff2
    };

    // Expand this as necessary for your inter stage buffer sizes
    static const unsigned interStageBuffSize[] = {
        StageWidth, /* Stage 0 - 1 */
        StageWidth, /* Stage 1 - 2 */
        StageWidth, /* Stage 2 - 3 */
        StageWidth, /* Stage 3 - 4 */
        StageWidth, /* Stage 4 - 5 */
        StageWidth, /* Stage 5 - 6 */
        StageWidth, /* Stage 6 - 7 */
        StageWidth, /* Stage 7 - 8 */
        StageWidth  /* Stage 8 - 9 */
    };

    typedef InOrderCPUParams Params;
    typedef RefCountingPtr<InOrderDynInst> DynInstPtr;

    //////////////////////////
    // RESOURCE SCHEDULING
    //////////////////////////
    struct ScheduleEntry {
        ScheduleEntry(int stage_num, int _priority, int res_num, int _cmd = 0,
                      int _idx = 0) :
            stageNum(stage_num), resNum(res_num), cmd(_cmd),
            idx(_idx), priority(_priority)
        { }
        virtual ~ScheduleEntry(){}

        // Stage number to perform this service.
        int stageNum;

        // Resource ID to access
        int resNum;

        // See specific resource for meaning
        unsigned cmd;

        // See specific resource for meaning
        unsigned idx;

        // Some Resources May Need Priority?
        int priority;
    };

    struct entryCompare {
        bool operator()(const ScheduleEntry* lhs, const ScheduleEntry* rhs) const
        {
            // Prioritize first by stage number that the resource is needed
            if (lhs->stageNum > rhs->stageNum) {
                return true;
            } else if (lhs->stageNum == rhs->stageNum) {
                /*if (lhs->resNum > rhs->resNum) {
                  return true;
                } else {
                  return false;
                  }*/

                if (lhs->priority > rhs->priority) {
                  return true;
                } else {
                  return false;
                }
            } else {
                return false;
            }
        }
    };


    typedef std::priority_queue<ScheduleEntry*, std::vector<ScheduleEntry*>,
                                         entryCompare> ResSchedule;

    void createFrontEndSchedule(DynInstPtr &inst);
    bool createBackEndSchedule(DynInstPtr &inst);
    int getNextPriority(DynInstPtr &inst, int stage_num);

    class InstStage {
      private:
        int nextTaskPriority;
        int stageNum;
        ResSchedule *instSched;

      public:
        InstStage(DynInstPtr inst, int stage_num);

        void needs(int unit, int request) {
            instSched->push( new ScheduleEntry(
                stageNum, nextTaskPriority++, unit, request
            ));
        }

        void needs(int unit, int request, int param) {
            instSched->push( new ScheduleEntry(
                stageNum, nextTaskPriority++, unit, request, param
            ));
        }

    };
};




#endif
