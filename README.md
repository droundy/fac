Bilge
=====

Bilge is a build system that tracks dependencies automatically.  We
track reads performed by the build commands in order to determine when
a file needs to be rebuilt.

# Caveats

1. Currently there is no tracking of readdir, so globbing etc will not
work reliably.

2. When we do track readdir, we will assume that the build command
only relies on file and directory names.  If the build command also
makes use of file sizes and/or modification times (or worse, access
times), the result could be an inconsistent build.
