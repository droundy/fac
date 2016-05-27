import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/stat.test'),
         th.reads(err, '/tmp/foo'),
         th.count_writes(err, 0),
         th.count_readdir(err, 0),
     ])
