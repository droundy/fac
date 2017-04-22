//! Hello

use std;
use std::io::{Read};
use std::hash::{Hasher};
use std::collections::hash_map::DefaultHasher;

#[cfg(unix)]
use std::os::unix::fs::{MetadataExt};

/// The stat information about a file
#[derive(Copy, Clone, PartialEq, PartialOrd, Eq, Hash)]
pub struct HashStat {
    /// modification time
    pub time: i64,
    /// in nanoseconds
    pub time_ns: i64,
    /// size
    pub size: u64,
    /// hash
    pub hash: u64,
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
        let hexit = if i[x] < 10 {
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
        let mut v = Vec::from(&encode(self.size)[..]);
        v.extend(&encode(self.time as u64)[..]);
        v.extend(&encode(self.time_ns as u64)[..]);
        v.extend(&encode(self.hash)[..]);
        v
    }
    /// decode from bytes
    pub fn decode(h: &[u8]) -> std::io::Result<HashStat> {
        if h.len() != 64 {
            Err(std::io::Error::new(std::io::ErrorKind::Other, "wrong size line!"))
        } else {
            Ok(HashStat {
                size: decode(h),
                time: decode(&h[16..]) as i64,
                time_ns: decode(&h[32..]) as i64,
                hash: decode(&h[48..]),
            })
        }
    }
}

/// stat a file
#[cfg(not(unix))]
pub fn stat(f: &std::path::Path) -> std::io::Result<HashStat> {
    let s = std::fs::metadata(f)?;
    Ok(HashStat {
        time: 0,
        time_ns: 0,
        size: s.len(),
        hash: 0,
    })
}
/// stat a file
#[cfg(unix)]
pub fn stat(f: &std::path::Path) -> std::io::Result<HashStat> {
    let s = std::fs::metadata(f)?;
    Ok(HashStat {
        time: s.mtime(),
        time_ns: s.mtime_nsec(),
        size: s.len(),
        hash: 0,
    })
}

/// hash a file
pub fn hash(f: &std::path::Path) -> std::io::Result<u64> {
    let mut file = std::fs::File::open(f)?;
    let mut contents = Vec::new();
    file.read_to_end(&mut contents)?;
    let mut h = DefaultHasher::new();
    h.write(&contents);
    Ok(h.finish())
}

/// hash and stat a file
pub fn hashstat(f: &std::path::Path) -> std::io::Result<HashStat> {
    let mut hs = stat(f)?;
    hs.hash = hash(f)?;
    Ok(hs)
}
