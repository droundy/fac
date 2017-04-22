//! Flags used by fac

extern crate clap;

const VERSION: &'static str = env!("CARGO_PKG_VERSION");

/// The flags determining build
pub struct Flags {
}

/// Parse command line arguments to determine what to do
pub fn args<'a>() -> Flags {
    clap::App::new("fac").version(VERSION).get_matches();
    Flags { }
}
