#!/usr/bin/python3

import argparse
import oqt


parser = argparse.ArgumentParser()
parser.add_argument("prfx", help="oqt src directory prfx")
parser.add_argument("orig_fn", help="original file name (within prfx)")
parser.add_argument("timestamp", help="original file timestamp")
parser.add_argument("diffs_location", help="location to download and read update osc.gz files")
parser.add_argument("initial_state", help="replication diff state consistent with original file")

args = parser.parse_args()

oqt.update.run_initial(args.prfx, args.orig_fn, args.timestamp, args.diffs_location, int(args.initial_state))
