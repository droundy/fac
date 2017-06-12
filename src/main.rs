//! Fac is a build system.

#![cfg_attr(feature = "strict", deny(warnings))]
#![cfg_attr(feature = "strict", deny(missing_docs))]

#[cfg(test)]
#[macro_use]
extern crate quickcheck;

#[macro_use]
extern crate clap;

#[cfg(feature="profile")]
extern crate cpuprofiler;

extern crate libc;
extern crate ctrlc;
extern crate metrohash;

mod git;
pub mod refset;

pub mod build;
mod version;

#[cfg(feature="profile")]
use cpuprofiler::PROFILER;

#[cfg(feature="profile")]
fn main() {
    let flags = build::flags::args();
    PROFILER.lock().unwrap().start("/tmp/fac.profile").unwrap();
    let exitcode = build::build(flags);
    PROFILER.lock().unwrap().stop().unwrap();
    std::process::exit(exitcode);
}

#[cfg(not(feature="profile"))]
fn main() {
    let flags = build::flags::args();
    let exitcode = build::build(flags);
    std::process::exit(exitcode);
}
