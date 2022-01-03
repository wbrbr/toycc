#!/usr/bin/python3
import sys
import os
import subprocess
from glob import glob

exit_code = 0
for f in glob("tests/end2end/*.c"):
    gcc_ret = subprocess.run(["gcc", f, "-o", "ref"]).returncode
    toycc_ret = subprocess.run(["./toycc", f]).returncode
    if gcc_ret != toycc_ret:
        print(f"Error: {f}")
        exit_code = 1
        continue

    if subprocess.run(["ld", "out.s"]).returncode != 0:
        print(f"Error: {f}")
        exit_code = 1
        continue

    ref_ret = subprocess.run(["./ref"]).returncode
    out_ret = subprocess.run(["./out"]).returncode
    if ref_ret != out_ret:
        print(f"Error: {f}")
        exit_code = 1
        continue

sys.exit(exit_code)
