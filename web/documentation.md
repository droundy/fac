# Documentation

Hello world

### To do:

6. Check that explicit inputs either exist and are in git or have rules.

7. Add syntax for specifying a cache directory, which will be ignored
   for both outputs and inputs.  sass is an example that uses such a
   cache directory that is likely to be in the tree.

- Kill jobs on SIGINT (attempted, but not quite working)
- Hashing of inputs
- Tracking of environment variables

- clean command?
- files to build on the command line

- Enable bilge to call bilge recursively (requires ptrace effort)

- Check if output files are outside the directory tree
- Create .gitignore files?

- Support "cache" directories
- Support for ~ as home directory? :(


<a href="building.pdf"><img src="building.png" alt="build times"></a>
<a href="rebuilding.pdf"><img src="rebuilding.png" alt="rebuild times"></a>
<a href="touching-c.pdf"><img src="touching-c.png" alt="more build times"></a>
<a href="touching-header.pdf"><img src="touching-header.png" alt="more build times"></a>
<a href="doing-nothing.pdf"><img src="doing-nothing.png" alt="more build times"></a>
