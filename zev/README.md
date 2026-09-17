# /zev

`/zev` is the ZevOS system-owned root directory.

On an installed ZevOS system, this directory is stored on the HDA-backed filesystem and contains ZevOS-owned system data/configuration that should not be mixed into `/etc`, `/usr`, or `/home`.

The live ISO currently mirrors `/zev` in the in-memory VFS. The persistent HDA filesystem driver and installer will populate the same layout when installed.
