#!/usr/bin/python3

import argparse
import oqt


parser = argparse.ArgumentParser()
parser.add_argument("prfx", help="oqt src directory prfx")


args = parser.parse_args()

oqt.update.run_update(args.prfx)
