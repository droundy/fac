//! Flags used by fac

use clap;

use std;
use std::env;
use std::path::{PathBuf};

use version;
use git;

/// The flags determining build
#[derive(Debug, Clone)]
pub struct Flags {
    /// We are asked to clean rather than build!
    pub clean: bool,
    /// Just show what we would do, do not actually do it.
    pub dry_run: bool,
    /// Keep rebuilding
    pub continual: bool,
    /// git add any files that need it
    pub git_add: bool,
    /// Print extra information
    pub verbosity: u64,
    /// Show command output even when they succeed
    pub show_output: bool,
    /// Where to save output logs
    pub log_output: Option<PathBuf>,
    /// Directory in which we were run
    pub run_from_directory: PathBuf,
    /// Git root
    pub root: PathBuf,

    /// number of jobs to run simultaneously
    pub jobs: usize,

    /// How strictly to check dependencies
    pub strictness: Strictness,

    /// What to build
    pub targets: Vec<PathBuf>,

    /// file to parse
    pub parse_only: Option<PathBuf>,
    /// requested makefile
    pub makefile: Option<PathBuf>,
    /// requested tupfile
    pub tupfile: Option<PathBuf>,
}

/// Parse command line arguments to determine what to do
pub fn args<'a>() -> Flags {
    let m = clap::App::new("fac")
        .version(version::VERSION)
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
        .arg(clap::Arg::with_name("dry")
             .long("dry")
             .help("dry run (don't do any building!)"))
        .arg(clap::Arg::with_name("verbose")
             .long("verbose")
             .short("v")
             .multiple(true)
             .help("show verbose output"))
        .arg(clap::Arg::with_name("show-output")
             .long("show-output")
             .short("V")
             .help("show command output"))
        .arg(clap::Arg::with_name("log-output")
             .long("log-output")
             .short("l")
             .takes_value(true)
             .value_name("LOG_DIRECTORY")
             .help("log command output to directory"))
        .group(clap::ArgGroup::with_name("command output")
               .arg("log-output")
               .arg("show-output")
        )
        .arg(clap::Arg::with_name("git-add")
             .long("git-add")
             .help("git add needed files"))
        .arg(clap::Arg::with_name("continual")
             .long("continual")
             .help("keep rebuilding"))
        .arg(clap::Arg::with_name("strict")
             .long("strict")
             .help("require strict dependencies, so first build will succeed"))
        .arg(clap::Arg::with_name("exhaustive")
             .long("exhaustive")
             .help("require exhaustive dependencies (makes --blind \"safe\")"))
        .group(clap::ArgGroup::with_name("strictness")
               .arg("strict")
               .arg("exhaustive")
        )
        .arg(clap::Arg::with_name("parse-only")
             .long("parse-only")
             .takes_value(true)
             .value_name("FACFILENAME")
             .help("just parse this .fac file"))
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
        .arg(clap::Arg::with_name("target")
             .index(1)
             .multiple(true)
             .help("names of files to build"))
        .get_matches();
    let here = env::current_dir().unwrap();
    let top = git::go_to_top();
    let strictness: Strictness;
    if m.is_present("strict") {
        strictness = Strictness::Strict;
    } else {
        strictness = Strictness::Normal;
    }
    let mut targets = Vec::new();
    if let Some(ts) = m.values_of_os("target") {
        for t in ts {
            let p = here.join(t);
            if let Ok(p) = p.strip_prefix(&top) {
                targets.push(PathBuf::from(p));
            } else {
                println!("Invalid path for target: {:?}", t);
                std::process::exit(1);
            }
        }
    }
    Flags {
        clean: m.is_present("clean"),
        dry_run: m.is_present("dry"),
        verbosity: m.occurrences_of("verbose"),
        show_output: m.is_present("show-output"),
        log_output: m.value_of("log-output").map(|s| PathBuf::from(s)),
        continual: m.is_present("continual"),
        git_add: m.is_present("git-add"),
        run_from_directory: here,
        root: top,
        jobs: value_t_or_exit!(m, "jobs", usize),
        strictness: strictness,
        targets: targets,
        parse_only: m.value_of("parse-only").map(|s| PathBuf::from(s)),
        makefile: m.value_of("makefile").map(|s| PathBuf::from(s)),
        tupfile: m.value_of("tupfile").map(|s| PathBuf::from(s)),
    }
}

/// Defines how strict we are about the facfile specifying all
/// dependencies.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Strictness {
    /// The usual lax default.  Users only specify what they care to.
    Normal,
    /// Users must specify at least one input to reflect any build
    /// dependency.
    Strict,
    /// Users must specify *all* dependencies, including all outputs
    /// that are also inputs.
    Exhaustive,
}
