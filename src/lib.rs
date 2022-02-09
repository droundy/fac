//! Fac is a build system.

#![cfg_attr(feature = "strict", deny(warnings))]
#![cfg_attr(feature = "strict", deny(missing_docs))]

extern crate clap;

/// A module with just the version in it.
pub mod version;
pub mod git;

pub mod build;
