import os

DIR = "/tmp/fuse-mount"

with open(os.path.join(DIR, "file_append.txt"), "a") as f:
    f.write("Hello world")
