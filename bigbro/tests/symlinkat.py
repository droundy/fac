import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/symlinkat.test'),
         th.writes(err, '/tmp/new-symlink'),
         th.writes(err, '/tmp/other-link'),
         th.count_writes(err, 2),
         th.count_readdir(err, 0),
     ])
