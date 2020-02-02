# read_compare
Read from device and compare with ISO image

For example, you can examine your CDROM with a specify ISO image to check it they are the same.

# Usage

```bash
$ make
$ ./read_compare /dev/sr0 ubuntu-18.04.3-desktop-amd64.iso
```

# Notice
This program compare by block with 2048 bytes.
