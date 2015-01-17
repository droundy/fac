# Documentation

Hello world

### To do:

1. Verify that inputs within the source directory are git added using `git ls-files`.

2. Around line 524 of build.c, we need to throw out outputs that were
   not human-specified, that only existed in a .done file.

3. It seems that bilge hangs when running latex (at least sometimes)

<a href="building.pdf"><img src="building.png" alt="build times"></a>
<a href="rebuilding.pdf"><img src="rebuilding.png" alt="rebuild times"></a>
<a href="touching-c.pdf"><img src="touching-c.png" alt="more build times"></a>
<a href="touching-header.pdf"><img src="touching-header.png" alt="more build times"></a>
<a href="doing-nothing.pdf"><img src="doing-nothing.png" alt="more build times"></a>
