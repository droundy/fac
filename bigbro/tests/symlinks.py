import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/symlinks.sh'),
         th.writes(err, '/tmp/subdir1/bar'),
         th.count_writes(err, 1),
         th.reads(err, '/tmp/root_symlink'),
         th.reads(err, '/tmp/subdir2/symlink'),
         th.reads(err, '/tmp/subdir1/foo_symlink'),
     ])
