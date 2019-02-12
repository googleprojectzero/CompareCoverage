/////////////////////////////////////////////////////////////////////////
//
// Author: Mateusz Jurczyk (mjurczyk@google.com)
//
// Copyright 2019 Google LLC
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// https://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "traces.h"

#include <inttypes.h>

#include "common.h"
#include "modules.h"

void Traces::TrySaveTrace(size_t pc, int trace_arg1, int trace_arg2) {
  // Form a wide (64-bit) representation of the trace for internal
  // deduplication.
  uint64_t trace = ConstructWideTrace(pc, trace_arg1, trace_arg2);

  // First line of deduplication - a set of traces operating on the binary
  // representation.
  if (traces_set_.find(trace) != traces_set_.end()) {
    return;
  }
  traces_set_.insert(trace);

  // Translate the instruction address to an executable image in memory.
  int mod_idx = modules_->GetModuleIndex(pc);
  if (mod_idx == -1) {
    Die("Failed to translate address %zx to an executable image, aborting.\n",
        pc);
  }

  // Construct an output trace (might be slightly different from a wide one) and
  // save it to be dumped to disk later.
  size_t output_trace = ConstructOutputTrace(
      pc - modules_->GetModuleBaseAddress(mod_idx),
      trace_arg1, trace_arg2);

  traces_list_.push_back(std::make_pair(mod_idx, output_trace));
}

int Traces::GetModulesCount() const {
  return modules_->GetModulesCount();
}

std::string Traces::GetModuleName(int idx) const {
  return modules_->GetModuleName(idx);
}

void Traces::GetTracesList(
    std::vector<std::pair<int, size_t>> *traces_list) const {
  *traces_list = traces_list_;
}

// 64-bit --> 32-bit hash function by Thomas Wang, source:
// http://www.concentric.net/~Ttwang/tech/inthash.htm
uint32_t Traces::Hash_64_32_Shift(uint64_t key) {
  key = (~key) + (key << 18);
  key = key ^ (key >> 31);
  key = key * 21;
  key = key ^ (key >> 11);
  key = key + (key << 6);
  key = key ^ (key >> 22);
  return key;
}

uint64_t Traces::ConstructWideTrace(
    size_t offset, int trace_arg1, int trace_arg2) {
  // The 64-bit wide trace format is as follows:
  //
  // Bits 60-63: Argument #1 specified by the caller.
  // Bits 48-59: Argument #2 specified by the caller.
  // Bits  0-54: Virtual address, or offset within a module.
  return static_cast<uint64_t>(offset) |
         (static_cast<uint64_t>(trace_arg2) << 48) |
         (static_cast<uint64_t>(trace_arg1) << 60);
}

size_t Traces::ConstructOutputTrace(
    size_t offset, int trace_arg1, int trace_arg2) {
#if WORDSIZE == 64
  // In x64, the output trace is equivalent to the wide one.
  return ConstructWideTrace(offset, trace_arg1, trace_arg2);
#else
  // In x86, we can't losslessly fit all information into the available 32 bits.
  // To reduce the data loss and improve the value distribution, we hash the
  // wide 64-bit trace into a smaller 32-bit one.
  return Hash_64_32_Shift(
      ConstructWideTrace(offset, trace_arg1, trace_arg2));
#endif
}
