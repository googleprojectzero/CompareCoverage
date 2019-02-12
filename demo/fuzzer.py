#########################################################################
#
# Author: Mateusz Jurczyk (mjurczyk@google.com)
#
# Copyright 2019 Google LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# https://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import glob
import os
import random
import string
import struct
import subprocess
import sys
import time

def PrettyHexPrint(data):
  for i in xrange(0, len(data), 16):
    sys.stdout.write("%.8x: " % i)

    for j in xrange(16):
      if i + j >= len(data):
        sys.stdout.write("   ")
      else:
        sys.stdout.write("%.2x " % ord(data[i + j]))

    for j in xrange(16):
      if i + j >= len(data):
        sys.stdout.write(" ")
      elif (ord(data[i + j]) >= 0x20) and (ord(data[i + j]) <= 0x7e):
        sys.stdout.write("%c" % data[i + j])
      else:
        sys.stdout.write(".")

    sys.stdout.write("\n")

def ParseSanitizerCoverage(dir_path, pid):
  sancov_files = glob.glob("%s/*.%d.sancov" % (dir_path, pid))
  if len(sancov_files) == 0:
    sys.exit("Expected SanitizerCoverage files on disk.")

  coverage = set()

  for log_file in sancov_files:
    with open(log_file, "rb") as f:
      data = f.read()
    os.remove(log_file)

    if len(data) < 8:
      sys.exit("Invalid SanitizerCoverage file found.")

    header = struct.unpack("<Q", data[:8])[0]

    wordsize = None
    fmt = None
    if header == 0xC0BFFFFFFFFFFF32:
      wordsize = 4
      fmt = "<I"
    elif header == 0xC0BFFFFFFFFFFF64:
      wordsize = 8
      fmt = "<Q"
    else:
      sys.exit("Invalid SanitizerCoverage header loaded.")

    for i in xrange(8, len(data), wordsize):
      coverage.add("%s+%x" %
                   (".".join(os.path.basename(log_file).split(".")[:-2]),
                    struct.unpack(fmt, data[i: i + wordsize])[0]))

  return coverage

def GetCoverageForInput(data, target):
  # Insert the ASAN_OPTIONS=coverage=1 environment variable so that coverage
  # information is written to disk.
  myenv = os.environ.copy()
  myenv["ASAN_OPTIONS"] = "coverage=1"

  # Run the program and pass the input data to it.
  process = subprocess.Popen(target, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             env=myenv)
  process.stdin.write(data)
  process.wait()

  return ParseSanitizerCoverage(dir_path=".", pid=process.pid)

def main(argv):
  if len(argv) != 2:
    print "Usage: %s <target program>" % argv[0]
    return

  # The program expects 57 bytes of input data.
  input_data = list(os.urandom(57))

  # Obtain the initial coverage information.
  coverage = GetCoverageForInput(data="".join(input_data), target=argv[1])

  print "---------- Initial coverage (%s, %d traces) ----------" %\
        (time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()), len(coverage))
  PrettyHexPrint(input_data)

  # Try to determine each byte of the input one by one.
  for i in xrange(len(input_data)):
    while True:
      # Set the byte at the current position to a random value.
      new_data = list(input_data)
      new_data[i] = chr(random.randint(0, 255))

      sample_cov = GetCoverageForInput(data="".join(new_data), target=argv[1])
      
      old_cov_size = len(coverage)
      coverage |= sample_cov

      if len(coverage) > old_cov_size:
        print "---------- New coverage (%s, %d traces) ----------" %\
              (time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()), len(coverage))
        PrettyHexPrint(new_data)
        print ""

        input_data = list(new_data)
        break

if __name__ == "__main__":
  main(sys.argv)
