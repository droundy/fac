# To do list

1. Figure out a clean solution to environment variables.  The current
   approach of blacklisting individual transient variables will not
   scale.  However, the scons approach of whitelisting environment
   variables that will be maintained has a tendency to be frustrating:
   developers don't know why things aren't working.  One option would
   be to let users create their own blacklists or whitelists.  That
   has the advantage of transparency, but places the burden of
   maintaining these lists on our users, which is silly.  Possibly we
   could support a default blacklist in /etc, which could be
   maintained by myself and distribution developers.  But that still
   seems like a poor choice.

1. Move build process to use only python3 and not python2.

1. Look into using libseccomp to optimize the ptrace usage by having
   only the system calls we want to trace generate events.

3. Port to macos.

3. Port to windows.
