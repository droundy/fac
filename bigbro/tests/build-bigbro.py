import tests.helper as th

def passes(out, err):
    return all(
        [th.reads(err, '/tests/build-bigbro.sh'),
         th.reads(err, '/tmp/bigbro.h'),
         th.writes(err, '/tmp/libbigbro.a'),
         th.writes(err, '/tmp/syscalls/linux.h'),
     ])
