#!/usr/bin/env python
# coding: utf-8

import glob
import os.path
import re

# The source for the Wren modules that are built into the VM or CLI are turned
# include C string literals. This way they can be compiled directly into the
# code so that file IO is not needed to find and read them.
#
# These string literals are stored in files with a ".wren.inc" extension and
# #included directly by other source files. This generates a ".wren.inc" file
# given a ".wren" module.

PREAMBLE = """// Generated automatically from {0}. Do not edit.
static const char* {1}ModuleSource =
{2};
"""

def wren_to_c_string(input_path, wren_source_lines, module):
  wren_source = ""
  for line in wren_source_lines:
    line = line.replace('"', "\\\"")
    line = line.replace("\n", "\\n\"")
    if wren_source: wren_source += "\n"
    wren_source += '"' + line

  return PREAMBLE.format(input_path, module, wren_source)


def main():

  input = "game.wren"
  ouput = "game.wren.inc"
  
  with open(input, "r") as f:
    wren_source_lines = f.readlines()

  module = os.path.splitext(os.path.basename(input))[0]

  c_source = wren_to_c_string(input, wren_source_lines, module)

  with open(ouput, "w") as f:
    f.write(c_source)


main()
