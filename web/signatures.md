# Signatures

I have begun looking into using gpg signatures to verify fac commits
and tags using git.  There doesn't seem to be a consensus approach on
how to use git for this purpose, so I am sort of figuring things out.
This document and this whole business is currently a work in
progress.

This document has a quick "how to" section, and then a discussion of
the issues, and my reasoning for this approach.

## The quick how-to

You clone the fac repository as usual.  If you are an expert, you can
clone a tagged version and then verify that the tag was signed by my
gpg key, which has the following fingerprint:

    284D 4AF3 EAF7 F01C 449C  24F5 9E5E E4A9 339E 788B

If you aren't an expert, you assume that you got a safe clone.  If you
are an expert, you also be asking me why you should trust that the bad
guys haven't modified the above fingerprint.  Either way, you just
have to hope for the best with regard to github's security (or find a
friend who knows me).

Once you have a good clone of the repository, you can run your git
commands with the wrapper script `git.py`

    ./git.py pull
    ./git.py log --show-signature

This will inform gpg to use a fac-specific keyring when it checks
signatures.  You need a pretty recent version of git to make pull
verify the signatures, more recent than I have.  So when the next
Debian release happens, you can expect this to be updated.  Right now,
your protection is pretty minimal.

## Rationalization

The concept of the pgp web of trust is broken, and also (even if it
worked) useless for determining trust with regard to software.  The
challenge of determining whether you trust me is pretty significant.
When you choose to compile my software, you are showing a great deal
of trust that I am not going to try to compromise your machine, and
also that I have not let someone else compromise *my* machine, who
might have the desire to compromise *your* machine.  Such trust is not
transitive.  And really the web of trust only involves trust that the
person's identity matches what their key says.  Just because my name
is David Roundy doesn't mean that you should let me install software
on your computer---which is the choice you are making when you choose
to compile and install fac.

The combination of git with gpg enables you to verify just one thing:
that given commits or tags were created by someone who has access to
a certain secret key.  They don't even do a very good job at that,
since by default signatures are checked against every key in your
public keychain, and conventional gnupg use involves not worrying
about the "danger" of fetching a public key, since "trust" is distinct
from having a copy of the key.  So what do you gain by my use of gnupg
to sign commits? You gain the knowledge that the same person (or
someone with access to my computer and passphrase) made the changes as
the guy who made the previous release.

Since this is realistically all you're likely to be able to discern
from my gnupg signatures, I have not uploaded my key to the
keyservers.  That would enable you to easily fetch it, but would also
encourage you to easily fetch any number of attackers' keys, and stick
them all together in the same spot.  Instead, I'm putting a public
keyring (with my public key) into the repository itself, along with a
handy script to run git using just this keyring.  Thus any signatures
by other keys will be rejected.

You wonder how to trust that this keyring hasn't been compromised?
Sorry, there's no solution to that, other than an out-of-band avenue
(such as possibly this webpage, see above).  If fac becomes famous, I
may publish abroad my gnupg fingerprint, so you can fetch it from mail
archives, etc, and hope that the NSA hasn't compromised them *all*.
If you know me personally (or want to meet me) my office is in 401B
Weniger Hall on Oregon State University's campus in Corvallis, Oregon.
You can visit with me and say "hi" and see if you've got the right
keyring.  You could try calling me on the phone, and then maybe you
could decide if my voice sounds trustworthy.  If you are able
determine trustworthiness over the phone, I suggest you find a job in
which you can make use of your super power.  Or just get an alter ego
and a superhero costume.

You might wonder how you can tell if the keyring is compromised some
time later.  Eventually, I'd like to make my script able to verify on
pull that any commit that modifies the keyring must be signed by a key
in the previously existing keyring.  Sadly, that sounds challenging.
For now, I'm waiting for the next Debian release, which should have a
git with the `--verify-signatures` option which enables it to refuse
commits that have not been signed.

