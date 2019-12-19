//! A file structure for holding hash and "stat" information about
//! files.  This is what fac uses to determine if a file has changed.

#[cfg(test)]
extern crate quickcheck;

use std;
use std::io::{Read};
use std::hash::{Hasher};
use metrohash::MetroHash64;

#[cfg(unix)]
use std::os::unix::fs::{MetadataExt};

use crate::build::{FileKind};

/// The stat information about a file
#[derive(Copy, Clone, PartialEq, PartialOrd, Eq, Hash, Debug)]
pub struct HashStat {
    /// modification time
    pub time: i64,
    /// in nanoseconds
    pub time_ns: i32,
    /// The file size.  We truncate more significant bits because we
    /// only care about size changing, and odds of a change by 4G are
    /// slim.
    pub size: u32,
    /// hash
    pub hash: u64,
    /// kind of file
    pub kind: Option<FileKind>,
}

#[cfg(test)]
impl quickcheck::Arbitrary for HashStat {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> HashStat {
        HashStat {
            time: i64::arbitrary(g),
            time_ns: i32::arbitrary(g),
            size: u32::arbitrary(g),
            hash: u64::arbitrary(g),
            kind: None,
        }
    }
}

#[cfg(test)]
quickcheck::quickcheck! {
    fn prop_encode_decode(hs: HashStat) -> bool {
        println!("original {:?}", hs);
        println!("encoded {:?}", hs.encode());
        println!("decoded {:?}", HashStat::decode(&hs.encode()));
        hs == HashStat::decode(&hs.encode())
    }
}

fn encode(i: u64) -> [u8; 16] {
    let mut out = [0;16];
    for x in 0..16 {
        let hexit = ((i >> (x*4)) & 15) as u8;
        if hexit < 10 {
            out[x] = b'0' + hexit;
        } else {
            out[x] = b'a' + (hexit-10);
        }
    }
    out
}

// fn decode(i: &[u8; 16]) -> u64 {
fn decode(i: &[u8]) -> u64 {
    let mut out = 0;
    for x in 0..16 {
        let hexit = if i[x] < b'a' {
            i[x] - b'0'
        } else {
            10 + i[x] - b'a'
        };
        out += (hexit as u64) << (x*4);
    }
    out
}

impl HashStat {
    /// encode as bytes
    pub fn encode(&self) -> Vec<u8> {
        let mut v = Vec::from(&encode(self.size as u64 + ((self.time_ns as u64) << 32))[..]);
        v.extend(&encode(self.time as u64)[..]);
        v.extend(&encode(self.hash)[..]);
        v
    }
    /// decode from bytes
    pub fn decode(h: &[u8]) -> HashStat {
        if h.len() != 3*16 {
            return HashStat {
                size: 0,
                time: 0,
                time_ns: 0,
                hash: 0,
                kind: None,
            };
        }
        let size_time = decode(h);
        HashStat {
            size: size_time as u32,
            time: decode(&h[16..]) as i64,
            time_ns: (size_time >> 32) as i32,
            hash: decode(&h[32..]),
            kind: None,
        }
    }
}

fn kind_of(m: &std::fs::Metadata) -> Option<FileKind> {
    if m.file_type().is_symlink() {
        Some(FileKind::Symlink)
    } else if m.file_type().is_dir() {
        Some(FileKind::Dir)
    } else if m.file_type().is_file() {
        Some(FileKind::File)
    } else {
        None
    }
}

/// stat a file
#[cfg(not(unix))]
pub fn stat(f: &std::path::Path) -> std::io::Result<HashStat> {
    let s = std::fs::symlink_metadata(f)?;
    Ok(HashStat {
        time: 0,
        time_ns: 0,
        size: s.len() as u32,
        hash: 0,
        kind: kind_of(&s),
    })
}
/// stat a file
#[cfg(unix)]
pub fn stat(f: &std::path::Path) -> std::io::Result<HashStat> {
    let s = std::fs::symlink_metadata(f)?;
    Ok(HashStat {
        time: s.mtime(),
        time_ns: (s.mtime_nsec()/10) as i32,
        size: s.len() as u32,
        hash: 0,
        kind: kind_of(&s),
    })
}

/// hash and stat a file
pub fn hashstat(f: &std::path::Path) -> std::io::Result<HashStat> {
    let mut hs = stat(f)?;
    hs.hash(f)?;
    Ok(hs)
}

impl HashStat {
    /// A HashStat that does not have any information.
    pub fn empty() -> HashStat {
        HashStat {
            time: 0,
            time_ns: 0,
            size: 0,
            hash: 0,
            kind: None,
        }
    }
    /// is the hash known?
    pub fn unfinished(&self) -> bool {
        self.hash == 0 || self.size == 0 || self.time == 0
    }
    /// look up any bits of the hashstat that we do not yet know.
    pub fn finish(&mut self, f: &std::path::Path) -> std::io::Result<()> {
        if self.size == 0 && self.time == 0 {
            match hashstat(f) {
                Ok(h) => {
                    *self = h;
                },
                Err(_) => {
                    // nothing to do
                }
            };
        } else if self.hash == 0 {
            self.hash(f)?;
        }
        Ok(())
    }
    /// try running sat
    pub fn stat(&mut self, f: &std::path::Path) -> std::io::Result<()> {
        if self.time == 0 && self.size == 0 {
            *self = stat(f)?;
        }
        Ok(())
    }
    /// see if it matches
    pub fn matches(&mut self, f: &std::path::Path, other: &HashStat) -> bool {
        self.stat(f).ok();
        if self.size != other.size {
            return false;
        }
        if self.time == other.time && self.time_ns == other.time_ns {
            return true;
        }
        if self.hash == 0 {
            self.finish(f).unwrap();
        }
        self.hash == other.hash
    }
    /// see if it we know matches without doing any disk IO
    pub fn cheap_matches(&mut self, other: &HashStat) -> bool {
        if self.size == 0 {
            return false;
        }
        self.size == other.size
            && self.time == other.time
            && self.time_ns == other.time_ns
    }
    /// hash a file
    fn hash(&mut self, f: &std::path::Path) -> std::io::Result<()> {
        let mut h = MetroHash64::new();
        match self.kind {
            Some(FileKind::File) => {
                let mut file = std::fs::File::open(f)?;
                let mut contents = Vec::new();
                file.read_to_end(&mut contents)?;
                h.write(&contents);
            },
            Some(FileKind::Dir) => {
                let mut entries = Vec::new();
                for entry in std::fs::read_dir(f)? {
                    entries.push(entry?.file_name());
                }
                entries.sort();
                for s in entries {
                    h.write(osstr_to_bytes(&s));
                }
            },
            Some(FileKind::Symlink) => {
                h.write(osstr_to_bytes(std::fs::read_link(f)?.as_os_str()));
            },
            None => (),
        }
        self.hash = h.finish();
        Ok(())
    }

}

use std::ffi::{OsStr};
#[cfg(unix)]
use std::os::unix::ffi::{OsStrExt};
/// Convert OsStr to bytes
#[cfg(unix)]
pub fn osstr_to_bytes(b: &OsStr) -> &[u8] {
    OsStr::as_bytes(b)
}

/// Convert OsStr to bytes
#[cfg(not(unix))]
pub fn osstr_to_bytes(b: &OsStr) -> &[u8] {
    b.to_str().unwrap().as_bytes()
}
