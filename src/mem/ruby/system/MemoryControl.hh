
/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
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
 */

/*
 * MemoryControl.h
 *
 * Description:  See MemoryControl.C
 *
 * $Id: $
 *
 */

#ifndef MEMORY_CONTROL_H
#define MEMORY_CONTROL_H

#include "mem/ruby/common/Global.hh"
#include "mem/gems_common/Map.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/profiler/Profiler.hh"
#include "mem/ruby/slicc_interface/AbstractChip.hh"
#include "mem/ruby/system/System.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include "mem/gems_common/util.hh"
#include "mem/ruby/system/MemoryNode.hh"
// Note that "MemoryMsg" is in the "generated" directory:
#include "mem/protocol/MemoryMsg.hh"
#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/system/AbstractMemOrCache.hh"

#include <list>

// This constant is part of the definition of tFAW; see
// the comments in header to MemoryControl.C
#define ACTIVATE_PER_TFAW 4

//////////////////////////////////////////////////////////////////////////////

class Consumer;

class MemoryControl : public Consumer, public AbstractMemOrCache {
public:

  // Constructors
  MemoryControl (AbstractChip* chip_ptr, int version);

  // Destructor
  ~MemoryControl ();

  // Public Methods

  void wakeup() ;

  void setConsumer (Consumer* consumer_ptr);
  Consumer* getConsumer () { return m_consumer_ptr; };
  void setDescription (const string& name) { m_name = name; };
  string getDescription () { return m_name; };

  // Called from the directory:
  void enqueue (const MsgPtr& message, int latency );
  void enqueueMemRef (MemoryNode& memRef);
  void dequeue ();
  const Message* peek ();
  MemoryNode peekNode ();
  bool isReady();
  bool areNSlotsAvailable (int n) { return true; };  // infinite queue length

  //// Called from L3 cache:
  //void writeBack(physical_address_t addr);

  void printConfig (ostream& out);
  void print (ostream& out) const;
  void setDebug (int debugFlag);

private:

  void enqueueToDirectory (MemoryNode req, int latency);
  int getBank (physical_address_t addr);
  int getRank (int bank);
  bool queueReady (int bank);
  void issueRequest (int bank);
  bool issueRefresh (int bank);
  void markTfaw (int rank);
  void executeCycle ();

  // Private copy constructor and assignment operator
  MemoryControl (const MemoryControl& obj);
  MemoryControl& operator=(const MemoryControl& obj);

  // data members
  AbstractChip* m_chip_ptr;
  Consumer* m_consumer_ptr;  // Consumer to signal a wakeup()
  string m_name;
  int m_version;
  int m_msg_counter;
  int m_awakened;

  int m_mem_bus_cycle_multiplier;
  int m_banks_per_rank;
  int m_ranks_per_dimm;
  int m_dimms_per_channel;
  int m_bank_bit_0;
  int m_rank_bit_0;
  int m_dimm_bit_0;
  unsigned int m_bank_queue_size;
  int m_bank_busy_time;
  int m_rank_rank_delay;
  int m_read_write_delay;
  int m_basic_bus_busy_time;
  int m_mem_ctl_latency;
  int m_refresh_period;
  int m_memRandomArbitrate;
  int m_tFaw;
  int m_memFixedDelay;

  int m_total_banks;
  int m_total_ranks;
  int m_refresh_period_system;

  // queues where memory requests live

  list<MemoryNode> m_response_queue;
  list<MemoryNode> m_input_queue;
  list<MemoryNode>* m_bankQueues;

  // Each entry indicates number of address-bus cycles until bank
  // is reschedulable:
  int* m_bankBusyCounter;
  int* m_oldRequest;

  uint64* m_tfaw_shift;
  int* m_tfaw_count;

  // Each of these indicates number of address-bus cycles until
  // we can issue a new request of the corresponding type:
  int m_busBusyCounter_Write;
  int m_busBusyCounter_ReadNewRank;
  int m_busBusyCounter_Basic;

  int m_busBusy_WhichRank;  // which rank last granted
  int m_roundRobin;         // which bank queue was last granted
  int m_refresh_count;      // cycles until next refresh
  int m_need_refresh;       // set whenever m_refresh_count goes to zero
  int m_refresh_bank;       // which bank to refresh next
  int m_ageCounter;         // age of old requests; to detect starvation
  int m_idleCount;          // watchdog timer for shutting down
  int m_debug;              // turn on printf's
};

#endif  // MEMORY_CONTROL_H
