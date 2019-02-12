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

#include "modules.h"

#include <inttypes.h>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#elif __linux__
#include <unistd.h>
#endif

#include <cstdio>
#include <unordered_map>

#include "common.h"

int Modules::GetModuleIndex(size_t address) {
  // Check the previously returned index first as an optimization.
  if (last_idx_ != -1 &&
      address >= modules_[last_idx_].base &&
      address < modules_[last_idx_].base + modules_[last_idx_].size) {
    return last_idx_;
  }

  // Because we don't expect that the traced process will consist of many
  // instrumented modules which will be jumping between themselves, we perform
  // a slow but simple O(n) search here.
  for (int i = 0; i < modules_.size(); i++) {
    if (address >= modules_[i].base &&
        address < modules_[i].base + modules_[i].size) {
      last_idx_ = i;
      return i;
    }
  }

  // If the address is not found in the cache, we have to update it with the new
  // module.
  return GetModuleIndexAndUpdateCache(address);
}

int Modules::GetModulesCount() const {
  return modules_.size();
}

size_t Modules::GetModuleBaseAddress(int idx) const {
  return modules_[idx].base;
}

std::string Modules::GetModuleName(int idx) const {
  return modules_[idx].name;
}

int Modules::GetModuleIndexAndUpdateCache(size_t address) {
#ifdef _WIN32
  // Translate an instruction address to the base address of the executable
  // image.
  HMODULE hmodule;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCSTR)address,
                          &hmodule)) {
    return -1;
  }

  // Request basic information about the image.
  MODULEINFO modinfo;
  if (!GetModuleInformation(GetCurrentProcess(), hmodule, &modinfo,
                            sizeof(modinfo))) {
    return -1;
  }

  // Obtain the image full path.
  char filepath[MAX_PATH];
  if (GetModuleFileNameA(hmodule, filepath, sizeof(filepath)) == 0) {
    return -1;
  }

  // Skip to the last backslash, which should be followed by the filename.
  char *filename = strrchr(filepath, '\\');
  if (filename != NULL) {
    filename++;
  } else {
    filename = filepath;
  }

  // Add the new module information to cache and return its index.
  ModuleInfo new_module;
  new_module.base = (size_t)modinfo.lpBaseOfDll;
  new_module.size = modinfo.SizeOfImage;
  new_module.name = filename;

  modules_.push_back(new_module);

  return modules_.size() - 1;

#elif __linux__

  // Scan through /proc/self/maps in search of the module.
  FILE *f = fopen("/proc/self/maps", "r");
  if (f == nullptr) {
    Die("Unable to open /proc/self/maps\n");
  }

  std::unordered_map<std::string, uint64_t> base_addresses;
  char line[512];
  while (!feof(f)) {
    if (fgets(line, sizeof(line), f) == nullptr) {
      break;
    }

    uint64_t address_start, address_end;
    char pathname[MAX_PATH + 1];

    // Parse a single line of the memory map.
    int elems = sscanf(
        line,
        "%" PRIx64 "-%" PRIx64 " %*s %*s %*s %*s %" TOSTRING(MAX_PATH) "[^\n]",
        &address_start, &address_end, pathname);

    // If we didn't parse out at least the start/end addresses (and potentially
    // also the pathname), skip the line.
    if (elems != 2 && elems != 3) {
      continue;
    }

    // If this is the first time we see this module, save its base address so we
    // can reference it later.
    if (base_addresses.find(pathname) == base_addresses.end()) {
      base_addresses[pathname] = address_start;
    }

    // Check if the address falls into the region address range.
    if (address < address_start || address >= address_end) {
      continue;
    }

    if (elems == 2) {
      // If the address falls into an unnamed region, construct a custom
      // pathname ourselves.
      snprintf(pathname, sizeof(pathname), "unknown_%" PRIx64, address_start);
    } else {
      // If there is a path name, look up the base address of the module.
      const auto it = base_addresses.find(pathname);
      if (it != base_addresses.end()) {
        address_start = it->second;
      }
    }

    // Extract the filename from the path.
    char *filename = strrchr(pathname, '/');
    if (filename != nullptr) {
      filename++;
    } else {
      filename = pathname;
    }

    // Add the new module information to cache and return its index.
    ModuleInfo new_module;
    new_module.base = address_start;
    new_module.size = address_end - address_start;
    new_module.name = filename;

    modules_.push_back(new_module);

    fclose(f);
    return modules_.size() - 1;
  }

  fclose(f);
  return -1;
#endif
}

