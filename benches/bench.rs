#![feature(test)]

extern crate test;
extern crate fac;

use std::path::PathBuf;
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
    fn env(b: &mut Bencher) {
        fac::build::env::hash();
        b.iter(|| fac::build::env::hash());
    }
}
