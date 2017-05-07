//! Flags used by fac

use clap;

use std::env;
use std::path::{PathBuf};

use git;

/// The flags determining build
#[derive(Debug, Clone)]
pub struct Flags {
    /// We are asked to clean rather than build!
    pub clean: bool,
    /// Keep rebuilding
    pub continual: bool,
    /// Print extra information
    pub verbose: bool,
    /// Show command output even when they succeed
    pub show_output: bool,
    /// Directory in which we were run
    pub run_from_directory: PathBuf,
    /// Git root
    pub root: PathBuf,

    /// number of jobs to run simultaneously
    pub jobs: usize,

    /// requested makefile
    pub makefile: Option<PathBuf>,
    /// requested tupfile
    pub tupfile: Option<PathBuf>,
}

/// Parse command line arguments to determine what to do
pub fn args<'a>() -> Flags {
    let m = clap::App::new("fac")
        .version(env!("CARGO_PKG_VERSION"))
        .about("build things")
        .arg(clap::Arg::with_name("jobs")
             .short("j")
             .long("jobs")
             .takes_value(true)
             .value_name("JOBS")
             .default_value("0")
             .hide_default_value(true)
             .help("the number of jobs to run simultaneously"))
        .arg(clap::Arg::with_name("clean")
             .short("c")
             .long("clean")
             .help("remove all traces of built files"))
        .arg(clap::Arg::with_name("verbose")
             .long("verbose")
             .short("v")
             .help("show verbose output"))
        .arg(clap::Arg::with_name("show-output")
             .long("show-output")
             .short("V")
             .help("show command output"))
        .arg(clap::Arg::with_name("continual")
             .long("continual")
             .help("keep rebuilding"))
        .arg(clap::Arg::with_name("makefile")
             .long("makefile")
             .takes_value(true)
             .value_name("MAKEFILE")
             .help("create a makefile"))
        .arg(clap::Arg::with_name("tupfile")
             .long("tupfile")
             .takes_value(true)
             .value_name("TUPFILE")
             .help("create a tupfile"))
        .get_matches();
    let here = env::current_dir().unwrap();
    let top = git::go_to_top();
    Flags {
        clean: m.is_present("clean"),
        verbose: m.is_present("verbose"),
        show_output: m.is_present("show-output"),
        continual: m.is_present("continual"),
        run_from_directory: here,
        root: top,
        jobs: value_t_or_exit!(m, "jobs", usize),
        makefile: m.value_of("makefile").map(|s| PathBuf::from(s)),
        tupfile: m.value_of("tupfile").map(|s| PathBuf::from(s)),
    }
}
