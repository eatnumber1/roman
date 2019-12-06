# Roman: A ROM MANager

Roman is a toolkit for doing ROM management. It is presented as a series of
lightweight tools which allow for complete ROM management by composing them
together.

In addition, Roman supports cloud file systems by accessing all files through
[RClone]. RClone is a mandatory dependency, but by leveraging it, ROMs stored in
cloud file systems can be verified extremely quickly. On my machine, I can
verify 24 thousand files in 30 seconds.

It is still undergoing active development, and is missing large pieces of
functionality. Your mileage <s>may vary</s> will be short.

[TOC]

## The DATPB File Format

A datpb is a binary [protobuf] generated from a [dat file]. You can create one
by either using [fetch] or [convert].

## Tools

### fetch
```
Usage: roman fetch [options] console

Fetch a datpb from redump.org. Writes the binary datpb to stdout.

The console argument must be a valid console as listed at
http://redump.org/downloads/. It is the system part of the datfile URL, so if
the URL to download "Commodore Amiga CD" is http://redump.org/datfile/acd/, the
correct argument is "acd".

Also see rclone --help for additional options.

Example usage:
$ roman fetch ps2 > ps2.datpb
```

### convert
Not yet implemented.

### verify
```
Usage: roman verify [options] datpb fs:path

Verify the files in a directory against a datpb.

Verify requires a running RClone instance with its remote control interface
enabled. See https://rclone.org/.

Arguments:
  datpb - A datpb file describing valid ROMs.
  fs:path - A RClone-style path specifier. The fs portion specifies the RClone
    filesystem to access, path specifies the path on the remote.

The --rclone_url flag is required, and describes how to connect to RClone. It
must specify both the URL to RClone, along with the username and password needed
to perform authenticated operations. For example,
--rclone_url='http://user:pass@localhost:5572'

Also see rclone --help for additional options.

Example usage:
$ rclone rcd --rc-user=user --rc-pass=pass &
$ roman verify --rclone_url='http://user:pass@localhost:5572' --recursive pce.datpb 'gdrive:/Games/PC Engine'
```

[dat file]: https://github.com/RetroPie/RetroPie-Setup/wiki/Validating,-Rebuilding,-and-Filtering-ROM-Collections#dat-files-the-cornerstone
[protobuf]: https://developers.google.com/protocol-buffers
[fetch]: #fetch
[convert]: #convert
[RClone]: https://rclone.org
