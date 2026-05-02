import os
import random
import string

MOUNT = "/tmp/fuse-mount"


def random_name(n=8):
    return "".join(random.choices(string.ascii_lowercase, k=n))


def random_bytes(n):
    return bytes(random.choices(range(256), k=n))


# Shadow model: filename -> bytes
shadow = {}

operations = ["create", "write", "read", "delete", "ls"]
weights = [1,         5,       4,      1,        2]


for step in range(100000):
    op = random.choices(operations, weights=weights)[0]
    print(f"step {step}: {op}")

    if op == "create":
        name = random_name()
        path = f"{MOUNT}/{name}"
        try:
            open(path, "wb").close()
            shadow[name] = b""
        except FileExistsError:
            pass

    elif op == "write" and shadow:
        name = random.choice(list(shadow))
        path = f"{MOUNT}/{name}"
        data = random_bytes(random.randint(1, 5000))
        with open(path, "ab") as f:           # append mode
            f.write(data)
        shadow[name] += data

    elif op == "read" and shadow:
        name = random.choice(list(shadow))
        path = f"{MOUNT}/{name}"
        with open(path, "rb") as f:
            actual = f.read()
        expected = shadow[name]
        assert actual == expected, (
            f"mismatch on {name}: got {len(actual)} bytes, "
            f"expected {len(expected)} bytes"
        )

    elif op == "delete" and shadow:
        name = random.choice(list(shadow))
        path = f"{MOUNT}/{name}"
        os.remove(path)
        del shadow[name]
    elif op == "ls":
        listed = set(os.listdir(MOUNT)) - {".", ".."}
        expected = set(shadow.keys())
        assert listed == expected, (
            f"ls mismatch: extra={listed - expected}, "
            f"missing={expected - listed}"
        )

print(f"completed {step+1} operations, {len(shadow)} files remain")
