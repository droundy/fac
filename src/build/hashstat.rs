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
