# nuts

> Like every good squirrel knows, you've got to protect your nuts.

-- attributed to [Kilgore Trout]

---

## Build

```
$ make
```

## Usage

```
$ ./nuts -h
Usage: ./nuts <command> <filename> <mntpoint>
```

### Create a file and mountpoint

```
$ ./nuts open foo.bar mnt_here
[WARN] There is no filename `foo.bar`... Create? [Y/n]:

Size of file [250MB]: 500MB
Select the filesystem type:

        1. ext2
        2. ext3
        3. ext4
        4. fat

Select: 3

[snipped for brevity]

Mounting to mnt_here/
Done!
```

Note that the `foo.bar` file was created, is an `ext4` filesystem and is the specified size (500GB not 500GiB):

```
$ file foo.bar
foo.bar: Linux rev 1.0 ext4 filesystem data, UUID=c1669e52-7454-49dc-9c92-43fc53710c70 (needs journal recovery) (extents) (64bit) (large files) (huge files)
$ ls -lh foo.bar
-rw-rw-r-- 1 btoll btoll 477M Jul 31 22:41 foo.bar
```

Lastly, note that the `mnt_here` mountpoint was indeed created (and mounted) for us:

```
$ ls
foo.bar  mnt_here  nuts
```

### Close a file and remove mountpoint

Give the program the same filename and mountpoint that was used for the `open` command:

```
$ ./nuts close foo.bar mnt_here
Encrypt? [Y/n] n
Unmounting mnt_here/
Done!
```

Note that the `mnt_here` mountpoint was unmounted and removed for us:

```
$ ls
foo.bar  nuts
```

Nice!

## Remove

```
$ make clean
```

## License

[GPLv3](COPYING)

## Author

Benjamin Toll

[Kilgore Trout]: https://en.wikipedia.org/wiki/Kilgore_Trout

