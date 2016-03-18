# Fac file format

$docnav

To configure this build system, you create one or more files ending
with `.fac`.  These files specify the rules to build your project,
and must be added to your git repository.  For most moderately complex
projects, you will have just one `.fac` file in git, which will
itself specify specify rules needed to create one or more additional
`.fac` files, which will contain the rules for doing the actual
build.  Each `.fac` file consists of:

1. Comments beginning with `"# "` (a pound sign followed by a space).

2. Rules beginning with `"| "` (a pipe character followed by a
   space).  The remainder of the line is the actual command to perform
   the build.  Following this line (possibly separated by blank lines
   and comments) are one or more of the following directives.  The
   order of these directives has no effect, so long as the follow the
   `"| "` line to which they apply.

2. Optional rules beginning with `"? "` are identical to rules
   beginning with a pipe, with the sole difference being that optional
   rules are only built if they are needed to build another target, or
   if they are explicitly requested.  Optional rules enable you to
   specify a large number of rules and only have what is needed built.

3. Output specifications beginning with `"> "` followed by the name of
   the file that is output.  There is no escaping performed, and only
   newlines and null characters are disallowed in the file name.
   There is little need to specify the output for rules, since fac
   can determine this automatically.  The only reason to specify
   output is so that on the very first build a user can request that
   we build only the specified rule.

4. Input specifications beginning with `"< "` followed by the name of
   the file that is required.  You only need specify inputs when they
   are generated files.  Even then, you need only specify the inputs
   if you wish to have the build succeed on the first attempt.

5. There are two ways to specify "cache" files.  A cache file is a
   file that may be either read or written by a given command, but
   doesn't affect the output and not itself an output.  One nice
   example is the ".pyc" files sometimes generated when you run a
   python program.  One python command may produce a .pyc file, and
   another may read it (if they both use the same module), but that
   does not mean that there is a dependency between those two
   commands.  You can specify a cache suffix (such as `.pyc`) using a
   `"c "` line, or you can specify a cache prefix (such as
   `/home/droundy/.ccache` or even better `~/.ccache`) using a
   capitalized `"C "` line.  The latter can be helpful if you get
   error messages stating that your rule is creating output outside
   your source tree.