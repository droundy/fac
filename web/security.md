# Fac security

$docnav

<img src="kells-fac-hires.svg" alt="Fac"/>

This document will define the security model for fac.  Although this
may seem excessive given the context, I believe every piece of
software should have a security model describing what it should and
should not do, and what an adversary should be able to control.

Fac's security model is pretty simple, since it is controlled by a
repository and typically has no interaction with the network.
Nevertheless there is value in making this explicit.

## Data flow

A threat model starts with a data flow diagram, indicating where data
moves in the application.  For fac this is very simple, and I will not
bother creating a visual model.  Information comes into fac through
the on-disk representation of a git repository.  This information
determines what fac does, which will necesarily involve running
arbitrary commands as requested by the configuration stored in the
repository.  This will in turn produce files on disk.

## Threats

### Other users

*A user without write permission in the repository or the `$PATH` (or
a superuser-like capability) should be unable to affect the commands
run by fac.* This does not mean that any commands run by fac must be
secure, which would be impossible, since fac can run arbitrary
insecure programs.  But said user should not be able to cause fac to
run the "wrong" command.

This could happen either through a tempfile vulnerability, or through
allowing other users to modify our processes.  Fac should be safe from
this kind of error due to not using tempfiles, except (in some
circumstances) in the repository directory, which is not writeable by
these attackers.

### Privilege escalation

*A user running fac in their own repository should be unable to
accomplish any action that they could not accomplish without the use
of fac.*  This means that if fac were to be implemented as an SUID
program, it would need to be secure.  Since it is not so implemented,
I don't see how it could lead to privilege escalation.

On MacOS, it is possible fac will need to be SUID root, in which case
this could become a significant risk.

### Network access

*Fac should not make or receive any network connections, unless such
 connections are explicitly created by a configured rule.*  If you
 create a rule such as

     | git clone https://github.com/droundy/fac

then of course fac will access the network, but absent any such rule,
fac should not be accessing the network.  No phoning home, for
instance.  This should essentially come for free, since we didn't
program this in.

## Conclusion

Because fac is designed to run arbitrary code, there is relatively
little we need worry about.  e.g. execution of arbitrary code due to a
malformed input file is a non-issue, since a well-formed input file
also enables the execution of arbitrary code.  The only two real
questions are privilege escalation and interference by other users
(most likely due to tempfile vulnerabilities).  We believe these risks
are adequately addressed by the current implementation of fac.
