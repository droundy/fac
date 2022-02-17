//! Flags used by fac

use clap;

use std;
use std::env;
use std::path::{PathBuf};

use crate::version;
use crate::git;

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
    /// run blind
    pub blind: bool,
    /// requested makefile
    pub makefile: Option<PathBuf>,
    /// requested tupfile
    pub tupfile: Option<PathBuf>,
    /// requested build.ninja file
    pub ninja: Option<PathBuf>,
    /// requested graphviz file
    pub dotfile: Option<PathBuf>,
    /// requested script
    pub script: Option<PathBuf>,
    /// requested tarball
    pub tar: Option<PathBuf>,
    /// Extra files to stick in the tarball
    pub include_in_tar: Vec<PathBuf>,
}

/// Parse command line arguments to determine what to do
pub fn args<'a>() -> Flags {
    let m = clap::Command::new("fac")
        .version(version::VERSION)
        .about("build things")
        .arg(clap::Arg::new("jobs")
             .short('j')
             .long("jobs")
             .takes_value(true)
             .value_name("JOBS")
             .default_value("0")
             .hide_default_value(true)
             .help("the number of jobs to run simultaneously"))
        .arg(clap::Arg::new("clean")
             .short('c')
             .long("clean")
             .help("remove all traces of built files"))
        .arg(clap::Arg::new("dry")
             .long("dry")
             .help("dry run (don't do any building!)"))
        .arg(clap::Arg::new("verbose")
             .long("verbose")
             .short('v')
             .multiple_occurrences(true)
             .help("show verbose output"))
        .arg(clap::Arg::new("show-output")
             .long("show-output")
             .short('V')
             .help("show command output"))
        .arg(clap::Arg::new("log-output")
             .long("log-output")
             .short('l')
             .takes_value(true)
             .value_name("LOG_DIRECTORY")
             .help("log command output to directory"))
        .group(clap::ArgGroup::new("command output")
               .arg("log-output")
               .arg("show-output")
        )
        .arg(clap::Arg::new("git-add")
             .long("git-add")
             .help("git add needed files"))
        .arg(clap::Arg::new("continual")
             .long("continual")
             .help("keep rebuilding"))
        .arg(clap::Arg::new("strict")
             .long("strict")
             .help("require strict dependencies, so first build will succeed"))
        .arg(clap::Arg::new("exhaustive")
             .long("exhaustive")
             .help("require exhaustive dependencies (makes --blind \"safe\")"))
        .group(clap::ArgGroup::new("strictness")
               .arg("strict")
               .arg("exhaustive")
        )
        .arg(clap::Arg::new("parse-only")
             .long("parse-only")
             .takes_value(true)
             .value_name("FACFILENAME")
             .help("just parse this .fac file"))
        .arg(clap::Arg::new("blind")
             .long("blind")
             .help("do not track dependencies"))
        .arg(clap::Arg::new("makefile")
             .long("makefile")
             .takes_value(true)
             .value_name("MAKEFILE")
             .help("create a makefile"))
        .arg(clap::Arg::new("tupfile")
             .long("tupfile")
             .takes_value(true)
             .value_name("TUPFILE")
             .help("create a tupfile"))
        .arg(clap::Arg::new("ninja")
             .long("ninja")
             .takes_value(true)
             .value_name("BUILD.NINJA")
             .help("create a build.ninja file"))
        .arg(clap::Arg::new("dotfile")
             .long("dotfile")
             .takes_value(true)
             .value_name("DOTFILE")
             .help("create a graphviz file to visualize dependencies"))
        .arg(clap::Arg::new("script")
             .long("script")
             .takes_value(true)
             .value_name("SCRIPTFILE")
             .help("create a build script"))
        .arg(clap::Arg::new("tar")
             .long("tar")
             .takes_value(true)
             .value_name("TARNAME.tar[.gz]")
             .help("create a tar archive"))
        .arg(clap::Arg::new("include-in-tar")
             .long("include-in-tar")
             .short('i')
             .takes_value(true)
             .value_name("FILENAME")
             .multiple_occurrences(true)
             .number_of_values(1)
             .help("include in tarball"))
        .arg(clap::Arg::new("target")
             .index(1)
             .allow_invalid_utf8(true)
             .multiple_occurrences(true)
             .help("names of files to build"))
        .get_matches();
    let here = env::current_dir().unwrap();
    let top = git::go_to_top();
    let strictness: Strictness;
    if m.is_present("strict") {
        strictness = Strictness::Strict;
    } else if m.is_present("exhaustive") {
        strictness = Strictness::Exhaustive;
    } else {
        strictness = Strictness::Normal;
    }
    let mut targets = Vec::new();
    if let Some(ts) = m.values_of_os("target") {
        for t in ts {
            let p = here.join(t);
            let p = if let Ok(p) = std::fs::canonicalize(&p) {
                p
            } else {
                p
            };
            if let Ok(p) = p.strip_prefix(&top) {
                targets.push(PathBuf::from(p));
            } else {
                println!("Invalid path for target: {:?}", t);
                std::process::exit(1);
            }
        }
    }
    let mut include_in_tar = Vec::new();
    if let Some(fs) = m.values_of_os("include-in-tar") {
        for f in fs {
            let p = here.join(f);
            let p = if let Ok(p) = std::fs::canonicalize(&p) {
                p
            } else {
                p
            };
            if let Ok(p) = p.strip_prefix(&top) {
                include_in_tar.push(PathBuf::from(p));
            } else {
                println!("Invalid path for including in tar: {:?}", f);
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
        jobs: clap::ArgMatches::value_of_t_or_exit(&m, "jobs"),
        strictness: strictness,
        targets: targets,
        parse_only: m.value_of("parse-only").map(|s| PathBuf::from(s)),
        blind: m.is_present("blind"),
        makefile: m.value_of("makefile").map(|s| PathBuf::from(s)),
        tupfile: m.value_of("tupfile").map(|s| PathBuf::from(s)),
        ninja: m.value_of("ninja").map(|s| PathBuf::from(s)),
        dotfile: m.value_of("dotfile").map(|s| PathBuf::from(s)),
        script: m.value_of("script").map(|s| PathBuf::from(s)),
        tar: m.value_of("tar").map(|s| PathBuf::from(s)),
        include_in_tar: include_in_tar,
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
