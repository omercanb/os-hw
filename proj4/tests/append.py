import os

DIR = "/tmp/fuse-mount"

with open(os.path.join(DIR, "file_append.txt"), "a") as f:
    f.write("Hello world"*100)
with open(os.path.join(DIR, "file_append.txt"), "r") as f:
    r = f.read(100_000)
    print(r)
