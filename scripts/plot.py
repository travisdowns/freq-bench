#!/usr/bin/env python3

import matplotlib.pyplot as plt
import csv
import argparse
import sys
import collections


p = argparse.ArgumentParser(
        usage='plot output from W_PLOT=1 ./bench')
p.add_argument('file', help='CSV file to plot (or stdin)', nargs='?')
p.add_argument('--out', help='output filename')
args = p.parse_args()
print("args = ", args)


if args.file:
    input = open(args.file, "r")
else:
    input = sys.stdin

columns = collections.defaultdict(list)

data = csv.reader(input, delimiter=',')
headers = [h for h in next(data) if h != '']
x_label = headers[0]
headers.pop(0)
print("headers are ", headers)
for row in data:
    for (i,v) in enumerate(row):
        if v != '': columns[i].append(float(v))

for i,c in columns.items():
    print("Column: ", i, "\n", c)

x = [str(int(v)) for v in columns[0]]

print("x = ", x)

fig, ax1 = plt.subplots(figsize=(10,7))
ax1.set_xlabel(x_label)
ax1.set_ylabel(headers[0])

ax2 = ax1.twinx()
ax2.set_ylabel('events')
# the next line sets both axis to use the same color cycle so we don't get dupe colors
ax2._get_lines.prop_cycler = ax1._get_lines.prop_cycler

for i,header in enumerate(headers, 1):
    y = columns[i]
    print("Column ", i, " y = ", y)
    ax = ax2 if i > 2 else ax1 
    ax.plot(x, y, label=header.strip(), linewidth=2, marker='.')

ax1.set_ylim(bottom=0)
plt.title('Performance of interleaved stores')
fig.legend(loc='upper right', ncol=2)

if (args.out):
    plt.savefig(args.out)
else:
    plt.show()
