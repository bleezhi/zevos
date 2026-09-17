`/bin` is the userspace program namespace.

The live build currently seeds these files into the RAM VFS at boot. An installed ZevOS filesystem will persist `/bin` on the HDA filesystem.

`/zev` is reserved for ZevOS-owned system data and is mirrored in the live VFS.
