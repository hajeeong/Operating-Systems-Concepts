import sys

memloc = 0

mode = sys.stdin.readline().rstrip()

while mode != "halt":
    if mode == "read":
        print(memloc)
        sys.stdout.flush()
    elif mode == "write":
        memloc = int(sys.stdin.readline().rstrip())
    mode = sys.stdin.readline().rstrip()
