//! Environment hashing

use std;
use std::hash::{Hasher, Hash};
use std::collections::hash_map::DefaultHasher;

/// VERBOSITY is used to enable our vprintln macro to know the
/// verbosity.  This is a bit ugly, but is needed due to rust macros
/// being hygienic.
static mut ENVHASH: u64 = 0;

/// Hash the environment
pub fn hash() -> u64 {
    if unsafe { ENVHASH } == 0 {
        let mut env: Vec<_> = std::env::vars_os().collect();
        env.sort();
        let mut hasher = DefaultHasher::new();
        env.hash(&mut hasher);
        unsafe { ENVHASH = hasher.finish(); }
    };
    unsafe { ENVHASH }
}
