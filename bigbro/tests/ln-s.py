import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/ln-s.sh'),
         th.writes(err, '/tmp-silly'),
         th.count_writes(err, 1),
     ])
