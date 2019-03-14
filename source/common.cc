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

#include "common.h"

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#endif

#include <cstdarg>
#include <cstdio>

void Die(const char *format, ...) {
  va_list args;
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  _exit(1);
}

int GetPid() {
#ifdef _WIN32
  return GetCurrentProcessId();
#elif __linux__
  return getpid();
#endif
}

