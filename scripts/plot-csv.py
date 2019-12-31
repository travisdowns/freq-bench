#!/usr/bin/env python3

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as plticker
import matplotlib.patches as patches
import pandas as pd
import numpy as np

import argparse
import csv
import collections
import itertools
import json
import os
import sys

# for arguments that should be comma-separate lists, we use splitlsit as the type
splitlist = lambda x: x.split(',')

p = argparse.ArgumentParser(usage='plot output from PLOT=1 ./bench')

# input and output file configuration
p.add_argument('input', help='CSV file to plot (or stdin)', nargs='*',
    type=argparse.FileType('r'), default=[ sys.stdin ])
p.add_argument('--out', help='output filename')

# input parsing configuration
p.add_argument('--sep', help='separator character (or regex) for input', default=',')

# column selection and configuration
p.add_argument('--xcols', help='Column index(es) to use as x axis (default: 0)', type=int, nargs='+')
p.add_argument('--xcols-by-name', help='Column name(s) to use as x axis', type=splitlist)
p.add_argument('--cols',  help='Use only these zero-based columns on primary axis (default: all columns)',
    type=int, nargs='+')
p.add_argument('--cols2', help='Use only these zero-based columns on secondary axis (default: no secondary axis)',
    type=int, nargs='+')
p.add_argument('--cols-by-name', help='Use only these comma-separated columns, specified by "name", i.e., the column header (default: all columns)',
    type=splitlist)
p.add_argument('--cols2-by-name', help='Use only these comma-separated columns, specified by "name", on the secondary axis',
    type=splitlist)
p.add_argument('--color-map', help='A JSON map from column name to color to use for that column',
    type=json.loads)
p.add_argument('--color', help='A list of colors to use for each column', type=splitlist)
p.add_argument('--color2', help='A list of colors to use for each secondary column', type=splitlist)

# chart labels and text
p.add_argument('--clabels', help="Comma separated list of column names used as label for data series (default: column header)",
    type=splitlist)
p.add_argument('--scatter', help='Do an XY scatter plot (default is a line plot with x values used only as labels)', action='store_true')
p.add_argument('--title', help='Set chart title', default='Some chart (use --title to specify title)')
p.add_argument('--xlabel', help='Set x axis label')
p.add_argument('--ylabel', help='Set y axis label')
p.add_argument('--ylabel2', help='Set the secondary y axis label')
p.add_argument('--suffix-names', help='Suffix each column name with the file it came from', action='store_true')

# legend
p.add_argument('--legend-loc', help='Set the legend location explicitly', type=str)

# data manipulation
p.add_argument('--jitter', help='Apply horizontal (x-axis) jitter of the given relative amount (default 0.1)',
    nargs='?', type=float, const=0.1)
p.add_argument('--group', help='Group data by the first column, with new min/median/max columns with one row per group')

# axis and line/point configuration
p.add_argument('--ylim', help='Set the y axis limits explicitly (e.g., to cross at zero)', type=float, nargs='+')
p.add_argument('--xlim', help='Set the x axis limits explicitly', type=float, nargs='+')
p.add_argument('--xrotate', help='rotate the xlablels by this amount', default=0)
p.add_argument('--tick-interval', help='use the given x-axis tick spacing (in x axis units)', type=int)
p.add_argument('--marker', help='use the given marker', type=splitlist)
p.add_argument('--marker2', help='use the given marker for the secondary axis', type=splitlist)
p.add_argument('--markersize', help='use the given marker size (or list of sizes)', type=splitlist)
p.add_argument('--markersize2', help='use the given marker size (or list of sizes) for secondary axis', type=splitlist)
p.add_argument('--alpha', help='use the given alpha for marker/line', type=float)
p.add_argument('--linewidth', help='use the given line width', type=float)
p.add_argument('--tight', help='use tight_layout for less space around chart', action='store_true')
p.add_argument('--allxticks', help="Force one x-axis tick for each value, disables auto ticks and may crowd x-axis", action='store_true')

# additional chart elements
p.add_argument('--patches', help='A JSON array of patches.Rectangle object arguments to patch onto the chart',
    type=json.loads)


# debugging
p.add_argument('--verbose', '-v', help='enable verbose logging', action='store_true')
args = p.parse_args()

vprint = print if args.verbose else lambda *a: None
vprint("args = ", args)
argsdict = vars(args)

# fix various random seeds so we get reproducible plots
# fix the mpl seed used to generate SVG IDs
mpl.rcParams['svg.hashsalt'] = 'foobar'

# numpy random seeds (used by e.g., jitter function below)
np.random.seed(123)

# if we are reading from stdin and stdin is a tty, print a warning since maybe the user just messed up
# the arguments and otherwise we just appear to hang
if (args.input and args.input[0] == sys.stdin):
    print("reading from standard input...", file=sys.stderr)

dfs = []
for f in args.input:
    df = pd.read_csv(f, sep=args.sep)
    if args.suffix_names:
        df = df.add_suffix(' ' + os.path.basename(f.name))
    vprint("----- df from: ", f.name, "-----\n", df.head(), "\n---------------------")
    dfs.append(df)

df = pd.concat(dfs, axis=1)
vprint("----- merged df -----\n", df.head(), "\n---------------------")

# renames duplicate columns by suffixing _1, _2 etc
class renamer():
    def __init__(self):
        self.d = dict()

    def __call__(self, x):
        if x not in self.d:
            self.d[x] = 0
            return x
        else:
            self.d[x] += 1
            return "%s_%d" % (x, self.d[x])


# rename any duplicate columns because otherwise Pandas gets mad
df = df.rename(columns=renamer())

vprint("---- renamed df ----\n", df.head(), "\n---------------------")

def col_names_to_indices(requested):
    colnames = [x.strip() for x in df.columns.tolist()]
    vprint("requested columns: ", requested, ", actual columns: ", colnames)
    cols = []
    for name in requested:
        if not name in colnames:
            exit("column name " + name + " not found, input columns were: " + ','.join(colnames))
        cols.append(colnames.index(name))
    return cols


def get_cols(arg_prefix, name, ret):
    numeric = argsdict[arg_prefix]
    string  = argsdict[arg_prefix + '_by_name']
    if numeric and string:
        sys.exit('both string {} and numeric {} columns provided for {}, not currently supported'.format(string, numeric, arg_prefix))
    if numeric:
        ret = numeric
    elif string:
        ret = col_names_to_indices(string)
    vprint('resolved', name, 'axis columns: ', ret)
    if (ret and max(ret) >= len(df.columns)):
        sys.exit("In", name, ", column", max(cols), "too large: input only has", len(df.columns), "columns", file=sys.stderr)
    return ret

df_all_idx = list(range(len(df.columns)))

# get the list of column indexes
xcols = get_cols('xcols', 'x primary', [0])
cols  = get_cols('cols',  'y primary', df_all_idx[1:])
cols2 = get_cols('cols2', 'y secondary', [])

assert set(cols).isdisjoint(xcols), "x and y column lists are not disjoint"
assert set(cols2).isdisjoint(xcols), "x and y column lists are not disjoint"

if len(xcols) > 1 and len(xcols) != len(cols):
    print('WARNING: x column count {} and y column count {} are different (and not 1) which is weird'.format(len(xcols), len(cols)))


# if legend_hack is True, we use apply the following hack
# (1) disable per-axis legend generation
# (2) generate the legend once at the figure level
# this works around pandas bugs which cause the legends to be incomplete when secondary_y
# is used
# as a side effect, however, the legend location isn't chosen automatically (i.e., the usual
# defualt of loc='best' isn't used), so you should set the legend location manually
legend_hack = cols2 is not None
vprint("using legend_hack = ", legend_hack)

if args.clabels:
    if len(df.columns) != len(args.clabels):
        sys.exit("ERROR: number of column labels " + str(len(args.clabels)) +
                " not equal to the number of selected columns " + str(len(df.columns)))
    df.columns = args.clabels

# dupes will break pandas beyond this point, should be impossible due to above renaming
dupes = df.columns.duplicated()
if True in dupes:
    print("Duplicate columns after merge, consider --suffix-names",
        df.columns[dupes].values.tolist(), file=sys.stderr)
    exit(1)

# do grouping (feature not complete)
if (args.group):
    vprint("before grouping\n", df)

    dfg = df.groupby(by=df.columns[0])

    df = dfg.agg([min, pd.DataFrame.median, max])

    vprint("agg\n---------------\n", df)

    df.columns = [tup[0] + ' (' + tup[1] + ')' for tup in df.columns.values]
    df.reset_index(inplace=True)

    vprint("flat\n---------------\n", df)

def jitter(arr, multiplier):
    stdev = multiplier*(max(arr)-min(arr))/len(arr)
    return arr if not len(arr) else arr + np.random.randn(len(arr)) * stdev

if args.jitter:
    for xi in xcols:
        df.iloc[:,xi] = jitter(df.iloc[:,xi], args.jitter)

kwargs = {}

if (legend_hack):
    # if we are using the legend hack, we'll generate the legend once at the figure level, so don't create one for each axes
    kwargs['legend'] = False

if (args.linewidth):
    kwargs["linewidth"] = args.linewidth

if args.color_map:
    sys.exit("FIXME: color-map doesn't work with one-by-one plotting")
    colors = []
    for i, cname in enumerate(df.columns):
        if i in xcols:
            continue
        if cname in args.color_map:
            vprint("Using color {} for column {}".format(args.color_map[cname], cname))
            colors.append(args.color_map[cname])
        else:
            print("WARNING no entry for column {} in given color-map".format(cname))
    vprint("colors = ", colors)
    kwargs["color"] = colors

if (args.scatter):
    kwargs['linestyle'] = 'none'
    # set a default marker if none specificed since otherwise nothing shows up!
    if not args.marker:
        args.marker = '.'

if (args.alpha):
    kwargs['alpha'] = args.alpha

# these are args that are basically just passed directly through to the plot command
# and generally correspond to matplotlib plot argumnets.
passthru_args  = ['markersize', 'marker', 'color']
passthru_args2 = ['markersize2', 'marker2', 'color2']

# populate the per-series arguments, based on the series index
def populate_args(idx, base, secondary):
    arglist = passthru_args2 if secondary else passthru_args
    kwargs = base.copy()
    for arg in arglist:
        argval = argsdict[arg]
        argname = arg[:-1] if secondary and arg.endswith('2') else arg
        if (argval):
            kwargs[argname] = argval[idx % len(argval)]
            vprint("set {} for {} col {} to {} (list: {})".format(argname, "secondary" if secondary else "primary", idx, kwargs[argname], argval))
        else:
            vprint("not set {} for col {}".format(arg, idx))
    return kwargs

# set x labels to strings so we don't get a scatter plot, and
# so the x labels are not themselves plotted
#if (args.scatter):
#    ax = df.plot.line(x=0, title=args.title, figsize=(12,8), grid=True, **kwargs)
#else:
    # df.iloc[:,xi] = df.iloc[:,xi].apply(str)

#fig, ax = plt.subplots(figsize=(12,8))

#ax.set_title(args.title)
#ax.grid(True)

assert len(xcols) <= len(cols)

ax = ax2 = None
for cols, secondary in [(cols, False), (cols2, True)]:
    for cidx, (xi, yi) in enumerate(zip(itertools.cycle(xcols),cols)):
        fullargs = populate_args(cidx, kwargs, secondary)
        vprint("{} series {}: xi={} yi={} kwargs={}".format('secondary' if secondary else 'primary', cidx, xi, yi, fullargs))
        ax_ = df.plot.line(ax=ax, x=xi, y=yi, title=args.title, figsize=(12,8), grid=True, secondary_y=secondary, **fullargs)
        if secondary:
            ax2 = ax_
        else:
            ax = ax_
        vprint("id(ax_)= ", id(ax_))

#ax = df.plot.line(x=0, title=args.title, figsize=(12,8), grid=True, **kwargs)

# this sets the ticks explicitly to one per x value, which means that
# all x values will be shown, but the x-axis could be crowded if there
# are too many
if args.allxticks:
    # we base the ticks on the first x-axis column
    ticks = df.iloc[:,xcols[0]].values
    plt.xticks(ticks=range(len(ticks)), labels=ticks)

if (args.tick_interval):
    ax.xaxis.set_major_locator(plticker.MultipleLocator(base=args.tick_interval))

if (args.xrotate):
    plt.xticks(rotation=args.xrotate)

if args.ylabel:
    ax.set_ylabel(args.ylabel)

if args.ylim:
    if (len(args.ylim) == 1):
        ax.set_ylim(args.ylim[0])
    elif (len(args.ylim) == 2):
        ax.set_ylim(args.ylim[0], args.ylim[1])
    else:
        sys.exit('provide one or two args to --ylim')

if args.xlim:
    if (len(args.xlim) == 1):
        ax.set_xlim(args.xlim[0])
    elif (len(args.xlim) == 2):
        ax.set_xlim(args.xlim[0], args.xlim[1])
    else:
        sys.exit('provide one or two args to --xlim')

# put some patches on
if args.patches:
    for p in args.patches:
        vprint('adding patch with args:', p)
        ax.add_patch(patches.Rectangle(**p))


# this needs to go after the ax2 handling, or else secondary axis x label will override
if args.xlabel:
    ax.set_xlabel(args.xlabel)

legargs = {}
if (args.legend_loc):
    legargs['loc'] = args.legend_loc

if legend_hack:
    leg = plt.gcf().legend(bbox_to_anchor=(0, 0, 1, 1), bbox_transform=ax.transAxes, **legargs)
else:
    # calling plt.legend() is a workaround for https://github.com/pandas-dev/pandas/issues/14958
    # otherwise markers don't show up except on the final series plotted
    leg = plt.legend()

if (args.alpha):
    # set legend symbols to be opaque, even if the plot markers aren't
    # https://stackoverflow.com/questions/12848808/set-legend-symbol-opacity-with-matplotlib
    for lh in leg.legendHandles:
        lh._legmarker.set_alpha(1)


vprint("all axes ", plt.gcf().get_axes())

if (args.tight):
    plt.tight_layout()

if (args.out):
    vprint("Saving figure to ", args.out, "...")
    plt.savefig(args.out)
else:
    vprint("Showing interactive plot...")
    plt.show()
