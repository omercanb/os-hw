#!/usr/bin/env python3
import os

DIR = "/tmp/fuse-mount"
N = 300

for i in range(N):
    with open(os.path.join(DIR, f"file{i + 1}.txt"), "w") as f:
        pass
        # f.write("hello\n")

print(f"created {N} files")
