#![feature(test)]

extern crate test;
extern crate fac;

use std::path::{PathBuf,Path};
use std::collections::HashMap;

fn create_hashmap(sz: usize) -> HashMap<PathBuf, usize> {
    let mut i = 0;
    let mut map = HashMap::new();
    for p in fac::git::ls_files() {
        map.insert(p, i);
        i += 1;
        if i == sz {
            return map;
        }
    }
    panic!("I cannot create a map this big!");
}

fn create_usize_hashmap(sz: usize) -> HashMap<usize, usize> {
    let mut i = 10;
    let mut map = HashMap::new();
    for p in 0..sz {
        map.insert(p, i);
        i += 1
    }
    map
}

#[cfg(test)]
mod tests {
    use super::*;
    use test::Bencher;

    #[bench]
    fn git_go_to_top(b: &mut Bencher) {
        b.iter(|| fac::git::go_to_top());
    }

    #[bench]
    fn git_dir(b: &mut Bencher) {
        b.iter(|| fac::git::git_dir());
    }

    #[bench]
    fn git_ls_files(b: &mut Bencher) {
        b.iter(|| fac::git::ls_files());
    }

    #[bench]
    fn access_hashmap_10(b: &mut Bencher) {
        let map = create_hashmap(10);
        let p = map.keys().next().unwrap();
        b.iter(|| map[p]);
    }

    #[bench]
    fn access_hashmap_100(b: &mut Bencher) {
        let map = create_hashmap(100);
        let p = map.keys().next().unwrap();
        b.iter(|| map[p]);
    }

    #[bench]
    fn access_usize_hashmap_10(b: &mut Bencher) {
        let map = create_usize_hashmap(10);
        let p = map.keys().next().unwrap();
        b.iter(|| map[p]);
    }

    #[bench]
    fn access_usize_hashmap_100(b: &mut Bencher) {
        let map = create_usize_hashmap(100);
        let p = map.keys().next().unwrap();
        b.iter(|| map[p]);
    }

    #[bench]
    fn env(b: &mut Bencher) {
        fac::build::env::hash();
        b.iter(|| fac::build::env::hash());
    }

    #[bench]
    fn hashstat_hosts(b: &mut Bencher) {
        b.iter(|| fac::build::hashstat::hashstat(&Path::new("/etc/hosts")));
    }

    #[bench]
    fn stat_hosts(b: &mut Bencher) {
        b.iter(|| fac::build::hashstat::stat(&Path::new("/etc/hosts")));
    }

    #[bench]
    fn hashstat_words(b: &mut Bencher) {
        b.iter(|| fac::build::hashstat::hashstat(&Path::new("/usr/share/dict/american-english")));
    }

    #[bench]
    fn stat_words(b: &mut Bencher) {
        b.iter(|| fac::build::hashstat::stat(&Path::new("/usr/share/dict/american-english")));
    }
}
