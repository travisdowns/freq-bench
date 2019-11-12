#!/usr/bin/env python2
# travis: generate events code for perf-event-timer
import os
import sys
import re

ocperf_dir = os.environ.get('OCPERF_DIR')
if not ocperf_dir:
    sys.exit('pmu-tools not found: set OCPERF_DIR env var to the path to pmu-tools')
if not os.path.isdir(ocperf_dir):
    sys.exit('OCPERF_DIR ({}) not found'.format(ocperf_dir))

sys.path.insert(0, ocperf_dir)
import ocperf

pattern = ('.*('
    'CPU_CLK_UNHALTED.*'
    '|INST_RETIRED_ANY'
    '|HW_IN'
    '|L1D_'
    '|MEM_(LOAD|INST)_RET'
    '|L2_RQ'
    '|UOPS_ISSUED_ANY'
    '|UOPS_DISPATCHED_PORT)')

emap = ocperf.find_emap()
if not emap:
    sys.exit("Unknown CPU or cannot find event table")

# header file has one const object for each event
header = open("perf-timer-events.hpp", "w")
header.write("#include \"perf-timer.hpp\"\n\n")
header.write("std::vector<PerfEvent> get_all_events();\n\n")

# cpp file has a function to return an array of all events
cpp = open("perf-timer-events.cpp", "w")
cpp.write('''
#include \"perf-timer-events.hpp\"\n\n
std::vector<PerfEvent> get_all_events() {
    static std::vector<PerfEvent> ALL = {
''')

for j in sorted(emap.events):
    varname = j.replace('.', '_').upper()
    if (re.match(pattern, varname)):
        header.write('const PerfEvent {:30} = PerfEvent( "{}", "{}" );\n'.format(varname, j, emap.events[j].output(noname=True)))
        cpp.write('        {:34},\n'.format(varname))

header.write('const PerfEvent NoEvent = {"",""};\n')
cpp.write('''
    };
    return ALL;
}''')
