//! Fac is a build system.

#![cfg_attr(feature = "strict", deny(warnings))]
#![cfg_attr(feature = "strict", deny(missing_docs))]

#[cfg(test)]
#[macro_use]
extern crate quickcheck;

#[macro_use]
extern crate clap;

extern crate libc;

mod git;
pub mod refset;

pub mod build;
mod version;

fn main() {
    let flags = build::flags::args();
    build::build(flags.clone(), |mut b| {
        b.build();
    });
}
