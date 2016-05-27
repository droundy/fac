import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/null-static.test'),
         th.count_reads(err, 1),
         th.count_writes(err, 0),
         th.count_readdir(err, 0),
     ])
