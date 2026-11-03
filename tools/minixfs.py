#!/usr/bin/env python3
"""
minixfs.py - build a small MINIX v1 file system image (as used by Linux 0.11),
without root rights or loop mounts.

  python3 tools/minixfs.py IMAGE BLOCKS ENTRY...

ENTRY is one of
  dir:/path                     directory (mode 0755)
  file:/path=HOSTFILE[:MODE]    regular file copied from HOSTFILE (octal MODE, default 0755)
  text:/path=TEXT               regular file with TEXT as content (mode 0644)
  chr:/path=MAJOR,MINOR         character device (mode 0666)

Parent directories must be listed before their contents. Layout and structures
follow include/linux/fs.h: 1024-byte blocks, 32-byte inodes, 16-byte directory
entries with 14-character names, magic 0x137F, zone size = block size.
Files can use the 7 direct zones and the single indirect zone (up to 519 KB).
"""
import struct
import sys

BLOCK = 1024
INODES_PER_BLOCK = BLOCK // 32
NAME_LEN = 14
SUPER_MAGIC = 0x137F
S_IFDIR, S_IFREG, S_IFCHR = 0o040000, 0o100000, 0o020000


class Node:
    def __init__(self, mode, data=b"", rdev=0):
        self.mode, self.data, self.rdev = mode, data, rdev
        self.entries = []          # directories: list of (name, Node)
        self.ino = 0
        self.nlinks = 1


def build(image, nblocks, specs):
    root = Node(S_IFDIR | 0o755)
    nodes = {"/": root}
    for spec in specs:
        kind, rest = spec.split(":", 1)
        path, _, arg = rest.partition("=")
        parent, name = path.rsplit("/", 1)
        parent = nodes[parent or "/"]
        if len(name.encode()) > NAME_LEN:
            sys.exit("name too long: " + path)
        if kind == "dir":
            node = Node(S_IFDIR | 0o755)
        elif kind == "file":
            host, _, mode = arg.partition(":")
            node = Node(S_IFREG | int(mode or "755", 8), open(host, "rb").read())
        elif kind == "text":
            node = Node(S_IFREG | 0o644, arg.encode().decode("unicode_escape").encode())
        elif kind == "chr":
            major, minor = (int(x) for x in arg.split(","))
            node = Node(S_IFCHR | 0o666, rdev=(major << 8) | minor)
        else:
            sys.exit("unknown entry: " + spec)
        parent.entries.append((name, node))
        nodes[path] = node

    # number inodes (root = 1) and count links
    order = [root]
    for node in order:
        node.ino = len(order) if node is root else node.ino
        for name, child in node.entries:
            order.append(child)
            child.ino = len(order)
            if child.mode & S_IFDIR:
                node.nlinks += 1
    root.ino = 1
    for node in order:
        if node.mode & S_IFDIR:
            node.nlinks += 1        # "." (and the entry in the parent / ".." for root)

    ninodes = max(32, len(order) + 8)
    imap_blocks = (ninodes + 1 + 8 * BLOCK - 1) // (8 * BLOCK)
    zmap_blocks = (nblocks + 8 * BLOCK - 1) // (8 * BLOCK)
    inode_blocks = (ninodes + INODES_PER_BLOCK - 1) // INODES_PER_BLOCK
    first_data = 2 + imap_blocks + zmap_blocks + inode_blocks

    img = bytearray(nblocks * BLOCK)
    next_zone = [first_data]

    def alloc():
        z = next_zone[0]
        if z >= nblocks:
            sys.exit("image too small")
        next_zone[0] += 1
        return z

    def parent_of(target):
        for node in order:
            if any(c is target for _, c in node.entries):
                return node
        return root

    # directory contents
    for node in order:
        if node.mode & S_IFDIR:
            ents = [(".", node), ("..", parent_of(node))] + node.entries
            node.data = b"".join(struct.pack("<H14s", c.ino, n.encode())
                                 for n, c in ents)

    # write inodes and data
    itable = 2 + imap_blocks + zmap_blocks
    for node in order:
        zones = [0] * 9
        if node.mode & S_IFCHR:
            zones[0] = node.rdev
            size = 0
        else:
            size = len(node.data)
            nz = (size + BLOCK - 1) // BLOCK
            if nz > 7 + 512:
                sys.exit("file too big for this tool")
            data_zones = []
            for i in range(nz):
                z = alloc()
                img[z * BLOCK:z * BLOCK + BLOCK] = node.data[i * BLOCK:(i + 1) * BLOCK].ljust(BLOCK, b"\0")
                data_zones.append(z)
            zones[:min(nz, 7)] = data_zones[:7]
            if nz > 7:
                ind = alloc()
                zones[7] = ind
                rest = data_zones[7:]
                img[ind * BLOCK:ind * BLOCK + 2 * len(rest)] = struct.pack("<%dH" % len(rest), *rest)
        off = itable * BLOCK + (node.ino - 1) * 32
        img[off:off + 32] = struct.pack("<HHIIBB9H", node.mode, 0, size, 0, 0,
                                        node.nlinks, *zones)

    # bitmaps: bit 0 is reserved; bits past the end are marked used
    def setbit(base, bit):
        img[base * BLOCK + bit // 8] |= 1 << (bit % 8)

    for bit in range(0, len(order) + 1):
        setbit(2, bit)
    for bit in range(ninodes + 1, imap_blocks * 8 * BLOCK):
        setbit(2, bit)
    nzbits = nblocks - first_data + 1
    for bit in range(0, next_zone[0] - first_data + 1):
        setbit(2 + imap_blocks, bit)
    for bit in range(nzbits, zmap_blocks * 8 * BLOCK):
        setbit(2 + imap_blocks, bit)

    max_size = (7 + 512 + 512 * 512) * BLOCK
    sb = struct.pack("<HHHHHHIH", ninodes, nblocks, imap_blocks, zmap_blocks,
                     first_data, 0, max_size, SUPER_MAGIC)
    img[BLOCK:BLOCK + len(sb)] = sb
    open(image, "wb").write(img)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    build(sys.argv[1], int(sys.argv[2]), sys.argv[3:])
