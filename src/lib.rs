//! Fac is a build system.

#![cfg_attr(feature = "strict", deny(warnings))]
#![cfg_attr(feature = "strict", deny(missing_docs))]

#[cfg(test)]
#[macro_use]
extern crate quickcheck;

#[macro_use]
extern crate clap;
extern crate ctrlc;
extern crate metrohash;
extern crate notify;
extern crate termcolor;
extern crate atty;
#[macro_use]
extern crate lazy_static;
extern crate tinyset;
extern crate crude_profiler;

/// A module with just the version in it.
pub mod version;
pub mod git;

pub mod build;
