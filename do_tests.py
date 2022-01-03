#!/usr/bin/python3
import sys
import subprocess
from glob import glob

exit_code = 0
for f in glob("tests/end2end/*.c"):
    gcc_ret = subprocess.run(["gcc", f, "-o", "ref"]).returncode
    toycc_ret = subprocess.run(["./toycc", f], capture_output=True).returncode
    if gcc_ret != toycc_ret:
        print(f"Compilation error: {f}")
        exit_code = 1
        continue

    if subprocess.run(["nasm", "-felf64", "out.s"]).returncode != 0:
        print(f"Error nasm: {f}")
        exit_code = 1
        continue

    if subprocess.run(["ld", "out.o", "-o", "out"]).returncode != 0:
        print(f"Error ld: {f}")
        exit_code = 1
        continue

    ref_ret = subprocess.run(["./ref"]).returncode
    out_ret = subprocess.run(["./out"]).returncode
    if ref_ret != out_ret:
        print(f"Error at runtime: {f}")
        exit_code = 1
        continue

sys.exit(exit_code)
