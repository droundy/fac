import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/git-clone.sh'),
         th.doesnt_ls(err, '/tmp/subdir1/bigbro/.git/refs'),
         th.doesnt_read(err, '/tmp/subdir1/bigbro/.git/HEAD'),
         th.writes(err, '/tmp/subdir1/bigbro/README.md'),
         th.writes(err, '/tmp/subdir1/bigbro/bigbro-linux.c'),
     ])
