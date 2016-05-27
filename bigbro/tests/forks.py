import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/forks.test'),
         th.writes(err, '/tmp/subdir1/deepdir/forks'),
         th.count_writes(err, 1),
         th.count_readdir(err, 0),
     ])
