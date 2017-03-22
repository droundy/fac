#!/bin/sh

set -ev

rm -rf $0.dir
mkdir $0.dir
cd $0.dir

cat > my-commit-hook <<EOF
#!/bin/sh

# Redirect output to stderr.
exec 1>&2

if test $(git diff --cached --name-only --diff-filter=A -z HEAD |
	  LC_ALL=C tr -d '[ -~]\0' | wc -c) != 0
then
	echo "Error: Attempt to add a non-ascii file name."
	exit 1
fi

# If there are whitespace errors, print the offending file names and fail.
echo git diff-index --check --cached HEAD --
exec git diff-index --check --cached HEAD --

EOF

cat > git-hook.fac <<EOF
| cp my-commit-hook .git/hooks/pre-commit

| cp  .git/hooks/pre-commit nice-hook
EOF
chmod +x my-commit-hook

git init
git add git-hook.fac my-commit-hook

git commit -am 'initial version'

../../fac || true

../../fac

./my-commit-hook

grep white nice-hook

echo '# stupid comment' >> my-commit-hook

git commit -am 'add nice hooks'

cat > newfile <<EOF
this has whitespace error   
EOF

git add newfile

rm .git/hooks/pre-commit
../../fac

if git commit -m 'bad new file'; then
    echo should have failed due to bad whitespace
    exit 1
fi

exit 0
