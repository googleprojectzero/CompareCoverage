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
// Description
// ===========
// 
// Definitions, constants and macros common to the overall cmpcov project.

#ifndef CMPCOV_COMMON_H_
#define CMPCOV_COMMON_H_

#include <inttypes.h>

#include <cstdlib>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif  // MAX_PATH

#if __LP64__ || defined(_WIN64)
#define WORDSIZE 64
#else
#define WORDSIZE 32
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Magic values found at the beginning of .sancov files to define their bitness.
// See https://clang.llvm.org/docs/SanitizerCoverage.html#sancov-data-format.
const uint64_t kMagic64 = 0xC0BFFFFFFFFFFF64ULL;
const uint64_t kMagic32 = 0xC0BFFFFFFFFFFF32ULL;
const uint64_t kMagic = WORDSIZE == 64 ? kMagic64 : kMagic32;

// Maximum length of instrumented string/memory buffers in calls to strcmp(),
// strncmp() and memcmp().
const size_t kMaxDataCmpLength = 32;

// Argument #1 for traces corresponding to memory comparisons. It is set to 15,
// as it is a reserved number that will never appear in the trace as the number
// of matching bytes in a single-variable comparison (which is limited to 8).
const int kMemcmpTraceArg1 = 15;

// Kills the process instantly on a critical error.
void Die(const char *format, ...);

// Returns the ID of the current process.
int GetPid();

#endif  // CMPCOV_COMMON_H_
