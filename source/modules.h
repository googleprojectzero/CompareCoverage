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
// The Modules class is designed to keep track of executable images loaded in
// the address space of the local process, and translate virtual addresses into
// the base+offset form.

#ifndef CMPCOV_MODULES_H_
#define CMPCOV_MODULES_H_

#include <cstdlib>
#include <string>
#include <vector>

// A descriptor of an executable module in the process address space.
struct ModuleInfo {
  size_t base;
  size_t size;
  std::string name;
};

class Modules {
 public:
  Modules() : last_idx_(-1) { }

  // Translates an address to a module index recognized by this class.
  int GetModuleIndex(size_t address);

  // Returns the total number of modules cached by this class.
  int GetModulesCount() const;

  // Returns the base address of an image associated with the given index.
  size_t GetModuleBaseAddress(int idx) const;

  // Returns the name of the image associated with the given index.
  std::string GetModuleName(int idx) const;

 private:
  // A list of modules known by the class.
  std::vector<ModuleInfo> modules_;

  // The last index returned by a call to GetModuleName, used for optimization.
  int last_idx_;

  // Obtains information about a module corresponding to a specific address from
  // the operating system, and adds it to the internal cache.
  int GetModuleIndexAndUpdateCache(size_t address);
};

#endif  // CMPCOV_MODULES_H_

