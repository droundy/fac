set -ev

cd tmp/root_symlink/tmp
echo foo > subdir2/symlink/bar

ls
pwd
cat subdir1/foo_symlink
