//! Fac is a build system.

#![cfg_attr(feature = "strict", deny(warnings))]
#![cfg_attr(feature = "strict", deny(missing_docs))]

#[cfg(test)]
#[macro_use]
extern crate quickcheck;

extern crate libc;

mod git;
pub mod refset;

pub mod build;

fn main() {
    git::go_to_top();
}
