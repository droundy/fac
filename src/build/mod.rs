//! Rules and types for building the stuff.

#[cfg(test)]
extern crate quickcheck;

extern crate typed_arena;
extern crate bigbro;

use std;
use std::io;

use std::ffi::{OsString, OsStr};
use std::path::{Path,PathBuf};

use std::collections::{HashSet, HashMap};

use std::io::{Read, Write};

use git;

pub mod hashstat;
pub mod flags;

/// VERBOSITY is used to enable our vprintln macro to know the
/// verbosity.  This is a bit ugly, but is needed due to rust macros
/// being hygienic.
static mut VERBOSITY: u64 = 0;

/// The `vprintln!` macro does a println! only if the --verbose flag
/// is specified.  It is written as a macro because if it were a
/// method or function then the arguments would be always evaluated
/// regardless of the verbosity (thus slowing things down).
/// Equivalently, we could write an if statement for each verbose
/// print, but that would be tedious.
macro_rules! vprintln {
    () => {{ if unsafe { VERBOSITY > 0 } { println!() } }};
    ($fmt:expr) => {{ if unsafe { VERBOSITY > 0 } { println!($fmt) } }};
    ($fmt:expr, $($arg:tt)*) => {{ if unsafe { VERBOSITY > 0 } { println!($fmt, $($arg)*) } }};
}

macro_rules! vvprintln {
    () => {{ if unsafe { VERBOSITY > 1 } { println!() } }};
    ($fmt:expr) => {{ if unsafe { VERBOSITY > 1 } { println!($fmt) } }};
    ($fmt:expr, $($arg:tt)*) => {{ if unsafe { VERBOSITY > 1 } { println!($fmt, $($arg)*) } }};
}

macro_rules! vvvprintln {
    () => {{ if unsafe { VERBOSITY > 2 } { println!() } }};
    ($fmt:expr) => {{ if unsafe { VERBOSITY > 2 } { println!($fmt) } }};
    ($fmt:expr, $($arg:tt)*) => {{ if unsafe { VERBOSITY > 2 } { println!($fmt, $($arg)*) } }};
}

/// `Id<'id>` is invariant w.r.t `'id`
///
/// This means that the inference engine is not allowed to shrink or
/// grow 'id to solve the borrow system.  This implementation is taken
/// from https://github.com/bluss/indexing/blob/master/src/lib.rs
#[derive(Copy, Clone, PartialEq, PartialOrd, Eq, Hash)]
struct Id<'id> { id: std::marker::PhantomData<*mut &'id ()>, }
impl<'id> Default for Id<'id> {
    #[inline]
    fn default() -> Self {
        Id { id: std::marker::PhantomData }
    }
}
impl<'id> std::fmt::Debug for Id<'id> {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.write_str("Id<'id>")
    }
}

/// The status of a rule.
#[derive(PartialEq, Eq, Hash, Copy, Clone, Debug)]
pub enum Status {
    /// This is the default status.
    Unknown,
    /// We are still deciding whether this needs to be built.
    BeingDetermined,
    /// We have determined that the rule doesn't need to run.
    Clean,
    /// The rule already ran.
    Built,
    /// The rule is currently being built.
    Building,
    /// The rule failed.
    Failed,
    /// This is used to indicate that specific rules are requested to
    /// be built.
    Marked,
    /// A rule cannot yet be built, because one of its inputs still
    /// needs to be built.
    Unready,
    /// This rule needs to be run.
    Dirty,
}

impl Status {
    /// This thing has been built or found to be clean
    pub fn is_done(self) -> bool {
        self == Status::Clean || self == Status::Built
    }
}

#[derive(PartialEq, Eq, Hash, Debug)]
struct StatusMap<T>( [T;9] );

impl<T> StatusMap<T> {
    fn new<F>(v: F) -> StatusMap<T>
        where F: Fn() -> T
    {
        StatusMap( [v(),v(),v(),v(),v(),v(),v(),v(),v()] )
    }
    #[cfg(test)]
    fn from(a: [T;9]) -> StatusMap<T> {
        StatusMap(a)
    }
}

impl<T: Clone> Clone for StatusMap<T> {
    fn clone(&self) -> Self {
        StatusMap( [self.0[0].clone(),
                    self.0[1].clone(),
                    self.0[2].clone(),
                    self.0[3].clone(),
                    self.0[4].clone(),
                    self.0[5].clone(),
                    self.0[6].clone(),
                    self.0[7].clone(),
                    self.0[8].clone()])
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for Status {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> Status {
        let choice: u32 = g.gen();
        match choice % 9 {
            0 => Status::Unknown,
            1 => Status::BeingDetermined,
            2 => Status::Clean,
            3 => Status::Built,
            4 => Status::Building,
            5 => Status::Failed,
            6 => Status::Marked,
            7 => Status::Unready,
            8 => Status::Dirty,
            _ => unimplemented!(),
        }
    }
}

#[cfg(test)]
impl<A: quickcheck::Arbitrary> quickcheck::Arbitrary for StatusMap<A> {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> StatusMap<A> {
        StatusMap::from([quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
                         quickcheck::Arbitrary::arbitrary(g),
        ])
    }
}

#[cfg(test)]
quickcheck! {
    fn prop_can_access_statusmap(m: StatusMap<bool>, s: Status) -> bool {
        m[s] || !m[s]
    }
}

#[cfg(test)]
quickcheck! {
    fn prop_status_eq(s: Status) -> bool {
        s == s
    }
}

impl<T> std::ops::Index<Status> for StatusMap<T>  {
    type Output = T;
    fn index(&self, s: Status) -> &T {
        &self.0[s as usize]
    }
}
impl<T> std::ops::IndexMut<Status> for StatusMap<T>  {
    fn index_mut(&mut self, s: Status) -> &mut T {
        &mut self.0[s as usize]
    }
}

/// Is the file a regular file, a symlink, or a directory?
#[derive(PartialEq, Eq, PartialOrd, Ord, Hash, Copy, Clone, Debug)]
pub enum FileKind {
    /// It is a regular file
    File,
    /// It is a directory
    Dir,
    /// It is a symlink
    Symlink,
}

/// A reference to a File
#[derive(PartialEq, Eq, Hash, Copy, Clone, Debug)]
pub struct FileRef<'id>(usize, Id<'id>);

/// A file (or directory) that is either an input or an output for
/// some rule.
#[derive(Debug)]
pub struct File<'id> {
    #[allow(dead_code)]
    id: Id<'id>,
    rule: Option<RuleRef<'id>>,
    path: PathBuf,
    // Question: could Vec be more efficient than RefSet here? It
    // depends if we add a rule multiple times to the same set of
    // children.  FIXME check this!
    children: HashSet<RuleRef<'id>>,

    rules_defined: Option<HashSet<RuleRef<'id>>>,

    hashstat: hashstat::HashStat,
    is_in_git: bool,
}

impl<'id> File<'id> {
    // /// Declare that this File is dirty (i.e. has been modified since
    // /// the last build).
    // pub fn dirty(&self) {
    //     for r in self.children.borrow().iter() {
    //         r.dirty();
    //     }
    // }

    /// Set file properties...
    pub fn stat(&mut self) -> io::Result<FileKind> {
        self.hashstat = hashstat::stat(&self.path)?;
        match self.hashstat.kind {
            Some(k) => Ok(k),
            None => Err(io::Error::new(io::ErrorKind::Other, "irregular file")),
        }
    }

    /// Is this `File` in git?
    pub fn in_git(&self) -> bool {
        self.is_in_git
    }

    /// Is this a fac file?
    pub fn is_fac_file(&self) -> bool {
        self.path.as_os_str().to_string_lossy().ends_with(".fac")
    }

    /// Does this thing exist?
    pub fn exists(&mut self) -> bool {
        self.stat().is_ok()
    }

    /// Remove it if it exists.
    fn unlink(&self) {
        std::fs::remove_file(&self.path).ok();
    }
}

/// A reference to a Rule
#[derive(PartialEq, Eq, Hash, Copy, Clone, Debug)]
pub struct RuleRef<'id>(usize, Id<'id>);

/// A rule for building something.
#[derive(Debug)]
pub struct Rule<'id> {
    #[allow(dead_code)]
    id: Id<'id>,
    inputs: Vec<FileRef<'id>>,
    outputs: Vec<FileRef<'id>>,
    all_inputs: HashSet<FileRef<'id>>,
    all_outputs: HashSet<FileRef<'id>>,
    hashstats: HashMap<FileRef<'id>, hashstat::HashStat>,

    status: Status,
    cache_prefixes: HashSet<OsString>,
    cache_suffixes: HashSet<OsString>,

    working_directory: PathBuf,
    facfile: FileRef<'id>,
    command: OsString,
    is_default: bool,
}

impl<'id> Rule<'id> {
    /// Identifies whether a given path is "cache"
    pub fn is_cache(&self, path: &Path) -> bool {
        self.cache_suffixes.iter().any(|s| is_suffix(path, s)) ||
            self.cache_prefixes.iter().any(|s| is_prefix(path, s))
    }
}

#[cfg(unix)]
use std::os::unix::ffi::{OsStrExt};
#[cfg(unix)]
fn is_suffix(path: &Path, suff: &OsStr) -> bool {
    let l = suff.as_bytes().len();
    let pp = path.as_os_str().as_bytes().len();
    pp > l && path.as_os_str().as_bytes()[pp-l..] == suff.as_bytes()[..]
}
#[cfg(unix)]
fn is_prefix(path: &Path, suff: &OsStr) -> bool {
    let l = suff.as_bytes().len();
    let p = path.as_os_str().as_bytes();
    p.len() > l && p[p.len()-l..] == suff.as_bytes()[..]
}

#[cfg(not(unix))]
fn is_suffix(path: &Path, suff: &OsStr) -> bool {
    let pathstring: String = path.as_os_str().to_string_lossy().into_owned();
    let suffstring: String = suff.to_string_lossy().into_owned();
    pathstring.ends_with(&suffstring)
}
#[cfg(not(unix))]
fn is_prefix(path: &Path, suff: &OsStr) -> bool {
    let pathstring: String = path.as_os_str().to_string_lossy().into_owned();
    let suffstring: String = suff.to_string_lossy().into_owned();
    pathstring.starts_with(&suffstring)
}

/// A struct that holds all the information needed to build.  You can
/// think of this as behaving like a set of global variables, but we
/// can drop the whole thing.
///
/// Build is implmented using a type witness `'id`, which makes it
/// impossible to create RuleRefs and FileRefs that are out of bounds.
/// I really should put some of these things in a "private" module so
/// that I can provide stronger guarantees of correctness in the main
/// build module.
#[derive(Debug)]
pub struct Build<'id> {
    #[allow(dead_code)]
    id: Id<'id>,
    files: Vec<File<'id>>,
    rules: Vec<Rule<'id>>,
    filemap: HashMap<PathBuf, FileRef<'id>>,
    rulemap: HashMap<(OsString, PathBuf), RuleRef<'id>>,
    statuses: StatusMap<HashSet<RuleRef<'id>>>,

    facfiles_used: HashSet<FileRef<'id>>,

    flags: flags::Flags,
}

/// Construct a new `Build<'id>` and use it to do some computation.
///
/// # Examples
///
/// ```
/// extern crate fac;
/// fac::build::build(fac::build::flags::args(), |b| {
///   println!("I am building {:?}", b)
/// });
/// ```
pub fn build<F, Out>(fl: flags::Flags, f: F) -> Out
    where F: for<'id> FnOnce(Build<'id>) -> Out
{
    unsafe { VERBOSITY = fl.verbosity; }
    // This approach to type witnesses is taken from
    // https://github.com/bluss/indexing/blob/master/src/container.rs
    let mut b = Build {
        id: Id::default(),
        files: Vec::new(),
        rules: Vec::new(),
        filemap: HashMap::new(),
        rulemap: HashMap::new(),
        statuses: StatusMap::new(|| HashSet::new()),
        facfiles_used: HashSet::new(),
        flags: fl,
    };
    for ref f in git::ls_files() {
        b.new_file_private(f, true);
    }
    f(b)
}

impl<'id> Build<'id> {
    /// Run the actual build!
    pub fn build(&mut self) -> i32 {
        if self.flags.parse_only.is_some() {
            let p = self.flags.run_from_directory.join(self.flags.parse_only
                                                       .as_ref().unwrap());
            let fr = self.new_file(&p);
            if let Err(e) = self.read_file(fr) {
                println!("Error: {}", e);
                return 1;
            }
            println!("finished parsing file {:?}", self.pretty_path_peek(fr));
            return 0;
        }
        let mut still_doing_facfiles = true;
        while still_doing_facfiles {
            println!("\nhandling facfiles");
            still_doing_facfiles = false;
            for f in self.filerefs() {
                if self[f].is_fac_file() && self[f].rules_defined.is_none() && self.is_file_done(f) {
                    println!("reading file {:?}", self.pretty_path(f));
                    self.read_file(f).unwrap();
                    still_doing_facfiles = true;
                    // self.print_fac_file(f).unwrap();
                } else if self[f].is_fac_file() && !self.is_file_done(f) {
                    println!("facfile {:?} is not done", self.pretty_path(f));
                } else if self[f].is_fac_file() {
                    println!("facfile {:?} already read with {} rules",
                             self.pretty_path(f),
                             self[f].rules_defined.as_ref().unwrap().len());
                }
            }
            self.mark_fac_files();
            let rules: Vec<_> = self.statuses[Status::Marked].iter().map(|&r| r).collect();
            for r in rules {
                self.check_cleanliness(r);
            }
            let rules: Vec<_> = self.statuses[Status::Dirty].iter().map(|&r| r).collect();
            for r in rules {
                still_doing_facfiles = true;
                if let Err(e) = self.run(r) {
                    println!("I got err {}", e);
                }
            }
        }
        self.save_factum_files().unwrap();
        if self.rulerefs().len() == 0 {
            println!("Please git add a .fac file containing rules!");
            std::process::exit(1);
        }

        // Now we start building the actual targets.
        self.mark_all();
        let rules: Vec<_> = self.statuses[Status::Marked].iter().map(|&r| r).collect();
        for r in rules {
            self.check_cleanliness(r);
        }
        while self.statuses[Status::Dirty].len() > 0
            || self.statuses[Status::Unready].len() > 0
        {
            let rules: Vec<_> = self.statuses[Status::Dirty].iter().map(|&r| r).collect();
            println!("\nhave {} dirty to build!", rules.len());
            for r in rules {
                if let Err(e) = self.run(r) {
                    println!("I got err {}", e);
                }
            }

            if self.statuses[Status::Dirty].len() == 0
                && self.statuses[Status::Unready].len() > 0
            {
                // Looks like we failed to build everything! There are
                // a few possibilities, including the possibility that
                // one of these unready rules is actually ready after
                // all.
                let rules: Vec<_>
                    = self.statuses[Status::Unready].iter().map(|&r| r).collect();
                for r in rules {
                    let mut need_to_try_again = false;
                    // FIXME figure out ignore_missing_files,
                    // happy_building_at_least_one, etc. from
                    // build.c:1171
                    let inputs: Vec<_> = self.rule(r).all_inputs.iter()
                        .map(|&i| i).collect();
                    for i in inputs {
                        if self[i].rule.is_none() && !self[i].is_in_git &&
                            !is_git_path(self.pretty_path_peek(i)) &&
                            self[i].path.starts_with(&self.flags.root)
                        {
                            if self[i].exists() {
                                let thepath = self[i].path.canonicalize().unwrap();
                                if thepath != self[i].path {
                                    // The canonicalization of the
                                    // path has changed! See issue #17
                                    // which this fixes. Presumably a
                                    // directory has been created or a
                                    // symlink modified, and the path
                                    // is now different.
                                    let t = self.new_file(thepath);
                                    // There is a small memory leak
                                    // here, since we don't free the
                                    // old target.  The trouble is
                                    // that we don't know if it is
                                    // otherwise in use, e.g. as the
                                    // output or input of a different
                                    // rule.  This *shouldn't* be
                                    // common, since once the path
                                    // exists, future runs will not
                                    // run into this leak.
                                    self.rule_mut(r).all_inputs.remove(&i);
                                    self.rule_mut(r).all_inputs.insert(t);
                                    // Now replace the target in the
                                    // vec of explicit inputs.
                                    for ii in self.rule_mut(r).inputs.iter_mut() {
                                        if *ii == i {
                                            *ii = t;
                                        }
                                    }
                                    need_to_try_again = true;
                                // } else if git_add_files {
                                //     git_add(r->inputs[i]->path);
                                //     need_to_try_again = true;
                                } else {
                                    println!("error: add {:?} to git, which is required for {}",
                                             self.pretty_path_peek(i),
                                             self.pretty_reason(r));
                                }
                            } else {
                                println!("error: missing file {:?}, which is required for {}",
                                         self.pretty_path_peek(i), self.pretty_reason(r));
                            }
                        }
                    }
                    if need_to_try_again {
                        self.check_cleanliness(r);
                        break;
                    } else {
                        self.failed(r);
                    }
                }
            }
        }
        self.save_factum_files().unwrap();
        self.summarize_build_results()
    }
    fn filerefs(&self) -> Vec<FileRef<'id>> {
        let mut out = Vec::new();
        for i in 0 .. self.files.len() {
            out.push(FileRef(i, self.id));
        }
        out
    }
    fn rulerefs(&self) -> Vec<RuleRef<'id>> {
        let mut out = Vec::new();
        for i in 0 .. self.rules.len() {
            out.push(RuleRef(i, self.id));
        }
        out
    }
    fn new_file_private<P: AsRef<Path>>(&mut self, path: P,
                                        is_in_git: bool)
                                        -> FileRef<'id> {
        let path = self.flags.root.join(path.as_ref());
        // If this file is already in our database, use the version
        // that we have.  It is an important invariant that we can
        // only have one file with a given path in the database.
        match self.filemap.get(&path) {
            Some(f) => return *f,
            None => ()
        }
        let f = FileRef(self.files.len(), self.id);
        self.files.push(File {
            id: self.id,
            rule: None,
            path: PathBuf::from(&path),
            children: HashSet::new(),
            rules_defined: None,
            hashstat: hashstat::HashStat::empty(),
            is_in_git: is_in_git,
        });
        self.filemap.insert(PathBuf::from(&path), f);
        f
    }

    /// Look up a `File` corresponding to a path, or if it doesn't
    /// exist, allocate space for a new `File`.
    ///
    /// # Examples
    ///
    /// ```
    /// use fac::build;
    /// build::build(build::flags::args(), |mut b| {
    ///   let t = b.new_file("test");
    /// })
    /// ```
    pub fn new_file<P: AsRef<Path>>(&mut self, path: P) -> FileRef<'id> {
        self.new_file_private(path, false)
    }

    /// Allocate space for a new `Rule`.
    pub fn new_rule(&mut self,
                    command: &OsStr,
                    working_directory: &Path,
                    facfile: FileRef<'id>,
                    cache_suffixes: HashSet<OsString>,
                    cache_prefixes: HashSet<OsString>,
                    is_default: bool)
                    -> RuleRef<'id> {
        let r = RuleRef(self.rules.len(), self.id);
        let key = (OsString::from(command), PathBuf::from(working_directory));
        assert!(!self.rulemap.contains_key(&key));
        self.rulemap.insert(key, r);
        self.rules.push(Rule {
            id: self.id,
            inputs: Vec::new(),
            outputs: Vec::new(),
            all_inputs: HashSet::new(),
            all_outputs: HashSet::new(),
            hashstats: HashMap::new(),
            status: Status::Unknown,
            cache_prefixes: cache_prefixes,
            cache_suffixes: cache_suffixes,
            working_directory: PathBuf::from(working_directory),
            facfile: facfile,
            command: OsString::from(command),
            is_default: is_default,
        });
        self.statuses[Status::Unknown].insert(r);
        r
    }

    /// Read a fac file
    pub fn read_file(&mut self, fileref: FileRef<'id>) -> io::Result<()> {
        self[fileref].rules_defined = Some(HashSet::new());
        let filepath = self[fileref].path.clone();
        let mut f = match std::fs::File::open(&filepath) {
            Ok(f) => f,
            Err(e) =>
                return Err(io::Error::new(e.kind(),
                                          format!("unable to open file {:?}: {}",
                                                  self.pretty_path_peek(fileref), e))),
        };
        let mut v = Vec::new();
        f.read_to_end(&mut v)?;
        let mut command: Option<RuleRef<'id>> = None;
        for (lineno_minus_one, line) in v.split(|c| *c == b'\n').enumerate() {
            let lineno = lineno_minus_one + 1;
            fn parse_error<T>(path: &Path, lineno: usize, msg: &str) -> io::Result<T> {
                Err(io::Error::new(io::ErrorKind::Other,
                                        format!("error: {:?}:{}: {}",
                                                path, lineno, msg)))
            }
            if line.len() < 2 || line[0] == b'#' { continue };
            if line[1] != b' ' {
                return parse_error(&filepath, lineno,
                                   "Second character of line should be a space.");
            }
            let get_rule = |r: Option<RuleRef<'id>>, c: char| -> io::Result<RuleRef<'id>> {
                match r {
                    None => parse_error(&filepath, lineno,
                                        &format!("'{}' line must follow '|' or '?'",c)),
                    Some(r) => Ok(r),
                }
            };
            match line[0] {
                b'|' => {
                    let r = self.new_rule(bytes_to_osstr(&line[2..]),
                                          filepath.parent().unwrap(),
                                          fileref,
                                          HashSet::new(),
                                          HashSet::new(),
                                          true);
                    self[fileref].rules_defined
                        .as_mut().expect("rules_defined should be some!").insert(r);
                    command = Some(r);
                },
                b'?' => {
                    command = Some(self.new_rule(bytes_to_osstr(&line[2..]),
                                                 filepath.parent().unwrap(),
                                                 fileref,
                                                 HashSet::new(),
                                                 HashSet::new(),
                                                 false));
                },
                b'>' => {
                    let f = self.new_file( &normalize(&filepath.parent().unwrap()
                                                      .join(bytes_to_osstr(&line[2..]))));
                    self.add_explicit_output(get_rule(command, '>')?, f);
                },
                b'<' => {
                    let f = self.new_file( &normalize(&filepath.parent().unwrap()
                                                      .join(bytes_to_osstr(&line[2..]))));
                    self.add_explicit_input(get_rule(command, '<')?, f);
                },
                b'c' => {
                    self.rule_mut(get_rule(command, 'c')?).cache_prefixes
                        .insert(bytes_to_osstr(&line[2..]).to_os_string());
                },
                b'C' => {
                    self.rule_mut(get_rule(command, 'C')?).cache_suffixes
                        .insert(bytes_to_osstr(&line[2..]).to_os_string());
                },
                _ => return parse_error(&filepath, lineno,
                                        &format!("Invalid first character: {:?}", line[0])),
            }
        }
        self.read_factum_file(fileref)
    }
    /// Read a factum file
    fn read_factum_file(&mut self, fileref: FileRef<'id>) -> io::Result<()> {
        let filepath = self[fileref].path.with_extension("fac.tum");
        let mut f = if let Ok(f) = std::fs::File::open(&filepath) {
            f
        } else {
            return Ok(()); // not an error for factum file to not exist!
        };
        let mut v = Vec::new();
        f.read_to_end(&mut v)?;
        let mut command: Option<RuleRef<'id>> = None;
        let mut file: Option<FileRef<'id>> = None;
        for (lineno_minus_one, line) in v.split(|c| *c == b'\n').enumerate() {
            let lineno = lineno_minus_one + 1;
            fn parse_error<T>(path: &Path, lineno: usize, msg: &str) -> io::Result<T> {
                Err(io::Error::new(io::ErrorKind::Other,
                                        format!("error: {:?}:{}: {}",
                                                path, lineno, msg)))
            }
            if line.len() < 2 || line[0] == b'#' { continue };
            if line[1] != b' ' {
                return parse_error(&filepath, lineno,
                                   "Second character of line should be a space.");
            }
            match line[0] {
                b'|' => {
                    let key = (bytes_to_osstr(&line[2..]).to_os_string(),
                               PathBuf::from(filepath.parent().unwrap()));
                    if let Some(r) = self.rulemap.get(&key) {
                        if self[fileref].rules_defined
                            .as_ref().expect("rules_defined should be some!").contains(r)
                        {
                            command = Some(*r);
                            file = None;
                        } else {
                            println!("mystery rule moved to different fac file?!");
                        }
                    } else {
                        println!("missing rule!");
                        command = None;
                    }
                },
                b'>' => {
                    let f = self.new_file(bytes_to_osstr(&line[2..]));
                    file = Some(f);
                    if let Some(r) = command {
                        self.add_output(r, f);
                    }
                },
                b'<' => {
                    let f = self.new_file(bytes_to_osstr(&line[2..]));
                    file = Some(f);
                    if let Some(r) = command {
                        self.add_input(r, f);
                    }
                },
                b'H' => {
                    if let Some(ff) = file {
                        if let Some(r) = command {
                            self.rule_mut(r)
                                .hashstats.insert(ff, hashstat::HashStat::decode(&line[2..]));
                        }
                    } else {
                        return parse_error(&filepath, lineno,
                                           &format!("H must be after a file!"));
                    }
                },
                _ => (),
            }
        }
        Ok(())
    }

    /// Write a fac file
    pub fn print_fac_file(&mut self, fileref: FileRef<'id>) -> io::Result<()> {
        if let Some(ref rules_defined) = self[fileref].rules_defined {
            for &r in rules_defined.iter() {
                println!("| {}", self.rule(r).command.to_string_lossy());
                for &i in self.rule(r).inputs.iter() {
                    println!("< {}", self[i].path.to_string_lossy());
                }
                for &o in self.rule(r).outputs.iter() {
                    println!("> {}", self[o].path.to_string_lossy());
                }
            }
        }
        Ok(())
    }
    /// Write factum files
    pub fn save_factum_files(&mut self) -> io::Result<()> {
        let facfiles: Vec<FileRef<'id>> = self.facfiles_used.drain().collect();
        for f in facfiles {
            self.save_factum_file(f).unwrap();
        }
        Ok(())
    }
    /// Write a fac.tum file
    pub fn save_factum_file(&mut self, fileref: FileRef<'id>) -> io::Result<()> {
        let mut f = std::fs::File::create(&self[fileref].path.with_extension("fac.tum"))?;
        if let Some(ref rules_defined) = self[fileref].rules_defined {
            for &r in rules_defined.iter() {
                f.write(b"\n| ")?;
                f.write(hashstat::osstr_to_bytes(&self.rule(r).command))?;
                f.write(b"\n")?;
                for &i in self.rule(r).all_inputs.iter() {
                    f.write(b"< ")?;
                    f.write(hashstat::osstr_to_bytes(self.pretty_path(i).as_os_str()))?;
                    if let Some(st) = self.rule(r).hashstats.get(&i) {
                        f.write(b"\nH ")?;
                        f.write(&st.encode())?;
                    }
                    f.write(b"\n")?;
                }
                for &o in self.rule(r).all_outputs.iter() {
                    f.write(b"> ")?;
                    f.write(hashstat::osstr_to_bytes(self.pretty_path(o).as_os_str()))?;
                    f.write(b"\nH ")?;
                    f.write(&self[o].hashstat.encode())?;
                    f.write(b"\n")?;
                }
            }
        }
        Ok(())
    }

    /// Add a new File as an input to this rule.
    pub fn add_input(&mut self, r: RuleRef<'id>, input: FileRef<'id>) {
        self.rule_mut(r).all_inputs.insert(input);
        self[input].children.insert(r);
    }
    /// Add a new File as an output of this rule.
    pub fn add_output(&mut self, r: RuleRef<'id>, output: FileRef<'id>) {
        self.rule_mut(r).all_outputs.insert(output);
        self[output].rule = Some(r);
    }

    /// Add a new File as an explicit input to this rule.
    fn add_explicit_input(&mut self, r: RuleRef<'id>, input: FileRef<'id>) {
        self.rule_mut(r).inputs.push(input);
        self.add_input(r, input);
    }
    /// Add a new File as an explicit output of this rule.
    fn add_explicit_output(&mut self, r: RuleRef<'id>, output: FileRef<'id>) {
        self.rule_mut(r).outputs.push(output);
        self.add_output(r, output);
    }

    /// Adjust the status of this rule, making sure to keep our sets
    /// up to date.
    pub fn set_status(&mut self, r: RuleRef<'id>, s: Status) {
        let oldstatus = self.rule(r).status;
        self.statuses[oldstatus].remove(&r);
        self.statuses[s].insert(r);
        self.rule_mut(r).status = s;
    }

    fn mark_fac_files(&mut self) {
        let mut to_mark = Vec::new();
        for &r in self.statuses[Status::Unknown].iter() {
            if self.rule(r).outputs.iter().any(|&f| self[f].is_fac_file()) {
                to_mark.push(r)
            }
        }
        for r in to_mark {
            self.set_status(r, Status::Marked);
        }
    }

    fn mark_all(&mut self) {
        let to_mark: Vec<_> = if self.flags.targets.len() > 0 {
            let mut m = Vec::new();
            let targs = self.flags.targets.clone();
            for p in targs {
                let f = self.new_file(&p);
                if let Some(r) = self[f].rule {
                    m.push(r);
                } else {
                    println!("error: no rule to make target {:?}", &p);
                    std::process::exit(1);
                }
            }
            m
        } else {
            self.statuses[Status::Unknown].iter()
                .map(|&r| r).filter(|&r| self.rule(r).is_default).collect()
        };
        for r in to_mark {
            self.set_status(r, Status::Marked);
        }
    }

    fn is_file_done(&self, f: FileRef<'id>) -> bool {
        if let Some(r) = self[f].rule {
            match self.rule(r).status {
                Status::Built | Status::Clean => true,
                _ => false,
            }
        } else {
            // No rule to build it, so it is inherently done!x
            true
        }
    }

    /// For debugging show all rules and their current status.
    pub fn show_rules_status(&self) {
        for r in self.rulerefs() {
            println!(">>> {} is {:?}", self.pretty_rule(r), self.rule(r).status);
        }
    }

    fn check_cleanliness(&mut self, r: RuleRef<'id>) {
        vvvprintln!("check_cleanliness {} (currently {:?})",
                    self.pretty_rule(r), self.rule(r).status);
        let old_status = self.rule(r).status;
        if old_status != Status::Unknown && old_status != Status::Unready &&
            old_status != Status::Marked {
                vvprintln!("     Already {:?}: {}", old_status, self.pretty_rule(r));
                return; // We already know if it is clean!
            }
        if self.rule(r).all_inputs.len() == 0 && self.rule(r).all_outputs.len() == 0 {
            // Presumably this means we have never built this rule, and its
            // inputs are in git.
            vvprintln!("     Never been built: {}", self.pretty_rule(r));
            self.set_status(r, Status::Dirty); // FIXME should sort by latency...
        }
        vvprintln!(" ??? Considering cleanliness of {}", self.pretty_rule(r));
        let mut rebuild_excuse: Option<String> = None;
        self.set_status(r, Status::BeingDetermined);
        let mut am_now_unready = false;
        let r_inputs: Vec<FileRef<'id>> = self.rule(r).inputs.iter().map(|&i| i).collect();
        let r_all_inputs: HashSet<FileRef<'id>> =
            self.rule(r).all_inputs.iter().map(|&i| i).collect();
        let r_other_inputs: HashSet<FileRef<'id>> = r_inputs.iter().map(|&i| i).collect();
        let r_other_inputs = &r_all_inputs - &r_other_inputs;
        for &i in r_all_inputs.iter() {
            if let Some(irule) = self[i].rule {
                match self.rule(irule).status {
                    Status::Unknown | Status::Marked => {
                        self.check_cleanliness(irule);
                    },
                    _ => (),
                };
                match self.rule(irule).status {
                    Status::Unknown | Status::Marked => {
                        panic!("This should not happen!?");
                    },
                    Status::BeingDetermined => {
                        // FIXME: nicer error handling would be great here.
                        println!("error: cycle involving {:?}, {:?}, and {:?}",
                                 self.pretty_rule(r), self.pretty_path(i),
                                 self.pretty_rule(irule));
                        std::process::exit(1);
                    },
                    Status::Dirty | Status::Unready | Status::Building => {
                        am_now_unready = true;
                    },
                    Status::Clean | Status::Failed | Status::Built => {
                        // Nothing to do here
                    },
                }
            }
        }
        self.set_status(r, old_status);
        if am_now_unready {
            vvprintln!(" !!! Unready for {}", self.pretty_rule(r));
            self.set_status(r, Status::Unready);
            return;
        }
        let mut is_dirty = false;

        // if (env.abc.a != r->env.abc.a || env.abc.b != r->env.abc.b || env.abc.c != r->env.abc.c) {
        //   if (r->env.abc.a || r->env.abc.b || r->env.abc.c) {
        //     rebuild_excuse(r, "the environment has changed");
        //   } else {
        //     rebuild_excuse(r, "we have never built it");
        //   }
        //   is_dirty = true;
        // }
        for &i in r_inputs.iter() {
            if !self[i].in_git() &&
                self[i].rule.is_none() &&
                self[i].path.starts_with(&self.flags.root) &&
                !is_git_path(self.pretty_path_peek(i)) {
                    // One of our explicit inputs is not in git, and
                    // we also do not know how to build it yet.  One
                    // hopes that there is some rule that will produce
                    // it!  :)
                    vvprintln!(" !!! Explicit input {:?} (i.e. {:?} not in git for {}",
                              self.pretty_path_peek(i), self[i].path,
                              self.pretty_rule(r));
                    self.set_status(r, Status::Unready);
                    return;
                }
        }
        for &i in r_other_inputs.iter() {
            self[i].stat().ok(); // check if it is a directory!
            if !self[i].in_git() &&
                self[i].rule.is_none() &&
                self[i].path.starts_with(&self.flags.root) &&
                !is_git_path(self.pretty_path_peek(i)) &&
                self[i].hashstat.kind != Some(FileKind::Dir) {
                    // One of our implicit inputs is not in git, and
                    // we also do not know how to build it.  But it
                    // was previously present and observed as an
                    // input.  This should be rebuilt since it may
                    // have "depended" on that input via something
                    // like "cat *.input > foo", and now that it
                    // doesn't exist we must rebuild.
                    rebuild_excuse = rebuild_excuse.or(
                        Some(format!("input {:?} has no rule",
                                     self.pretty_path_peek(i))));
                    is_dirty = true;
                }
        }
        if !is_dirty {
            for &i in r_all_inputs.iter() {
                let path = self[i].path.clone();
                self[i].hashstat.stat(&path);
                if let Some(istat) = self.rule(r).hashstats.get(&i).map(|s| *s) {
                    if let Some(irule) = self[i].rule {
                        if self.rule(irule).status == Status::Built {
                            if self[i].hashstat.cheap_matches(&istat) {
                                // nothing to do here
                            } else if self[i].hashstat.matches(&path, &istat) {
                                // the following handles updating the
                                // file stats in the rule, so next
                                // time a cheap match will be enough.
                                let newstat = self[i].hashstat;
                                self.rule_mut(r).hashstats.insert(i, newstat);
                                let facfile = self.rule(r).facfile;
                                self.facfiles_used.insert(facfile);
                            } else {
                                rebuild_excuse = rebuild_excuse.or(
                                    Some(format!("{:?} has been rebuilt",
                                                 self.pretty_path_peek(i))));
                                is_dirty = true;
                                break;
                            }
                            continue; // this input is fine
                        }
                    }
                    // We did not just build it, so different excuses!
                    if self[i].hashstat.cheap_matches(&istat) {
                        // nothing to do here
                    } else if self[i].hashstat.matches(&path, &istat) {
                        // the following handles updating the file
                        // stats in the rule, so next time a cheap
                        // match will be enough.
                        let newstat = self[i].hashstat;
                        self.rule_mut(r).hashstats.insert(i, newstat);
                        let facfile = self.rule(r).facfile;
                        self.facfiles_used.insert(facfile);
                    } else {
                        rebuild_excuse = rebuild_excuse.or(
                            Some(format!("{:?} has been modified",
                                         self.pretty_path_peek(i))));
                        is_dirty = true;
                        break;
                    }

                } else if self[i].hashstat.kind != Some(FileKind::Dir) {
                    // In case of an input that is a directory, if it
                    // has no input time, we conclude that it wasn't
                    // actually readdired, and only needs to exist.
                    // Otherwise, if there is no input time, something
                    // is weird and we must need to rebuild.
                    rebuild_excuse = rebuild_excuse.or(
                        Some(format!("have no information on {:?}",
                                     self.pretty_path_peek(i))));
                    is_dirty = true;
                    break;
                }
            }
        }
        if !is_dirty {
            let r_all_outputs: HashSet<FileRef<'id>> =
                self.rule(r).all_outputs.iter().map(|&o| o).collect();
            if r_all_outputs.len() == 0 {
                rebuild_excuse = rebuild_excuse.or(
                    Some(format!("it has never been run")));
                is_dirty = true;
            }
            for o in r_all_outputs {
                if let Some(ostat) = self.rule(r).hashstats.get(&o).map(|s| *s) {
                    let path = self[o].path.clone();
                    if self[o].hashstat.cheap_matches(&ostat) {
                        // nothing to do here
                    } else if self[o].hashstat.matches(&path, &ostat) {
                        // the following handles updating the file
                        // stats in the rule, so next time a cheap
                        // match will be enough.
                        let newstat = self[o].hashstat;
                        self.rule_mut(r).hashstats.insert(o, newstat);
                        let facfile = self.rule(r).facfile;
                        self.facfiles_used.insert(facfile);
                    } else if self[o].hashstat.kind == Some(FileKind::Dir) {
                        // If the rule creates a directory, we want to
                        // ignore any changes within that directory,
                        // there is no reason to rebuild just because
                        // the directory contents changed.
                    } else {
                        rebuild_excuse = rebuild_excuse.or(
                            Some(format!("output {:?} has been modified",
                                         self.pretty_path_peek(o))));
                        is_dirty = true;
                        break;
                    }
                } else {
                    rebuild_excuse = rebuild_excuse.or(
                        Some(format!("have never built {:?}",
                                     self.pretty_path_peek(o))));
                    is_dirty = true;
                    break;
                }
            }
        }
        if is_dirty {
            vprintln!(" *** Building {}\n     because {}",
                      self.pretty_rule(r),
                      rebuild_excuse.unwrap_or(String::from("I am confused?")));
            self.dirty(r);
        } else {
            vvprintln!(" *** Clean: {}", self.pretty_rule(r));
            self.set_status(r, Status::Clean);
            if old_status == Status::Unready {
                // If we were previously unready, let us now check if
                // any of our unready children are now ready.
                let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                    .flat_map(|o| self[*o].children.iter()).map(|c| *c)
                    .filter(|&c| self.rule(c).status == Status::Unready).collect();
                for childr in children {
                    self.check_cleanliness(childr);
                }
            }
        }
    }

    /// Mark this rule as dirty, adjusting other rules to match.
    fn dirty(&mut self, r: RuleRef<'id>) {
        let oldstat = self.rule(r).status;
        if oldstat != Status::Dirty {
            self.set_status(r, Status::Dirty);
            if oldstat != Status::Unready {
                // Need to inform marked child rules they are unready now
                let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                    .flat_map(|o| self[*o].children.iter()).map(|c| *c)
                    .filter(|&c| self.rule(c).status == Status::Marked).collect();
                // This is a separate loop to satisfy the borrow checker.
                for childr in children.iter() {
                    self.unready(*childr);
                }
            }
        }
    }
    /// Make this rule (and any that depend on it) `Status::Unready`.
    fn unready(&mut self, r: RuleRef<'id>) {
        if self.rule(r).status != Status::Unready {
            self.set_status(r, Status::Unready);
            // Need to inform marked child rules they are unready now
            let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                .flat_map(|o| self[*o].children.iter()).map(|c| *c)
                .filter(|&c| self.rule(c).status == Status::Marked).collect();
            // This is a separate loop to satisfy the borrow checker.
            // It is somewhat less efficient to store this, but such
            // is life.  I could have stored a HashSet rather than a
            // Vec, but suspect the hash table overhead would make it
            // less efficient.
            for childr in children.iter() {
                self.unready(*childr);
            }
        }
    }
    fn failed(&mut self, r: RuleRef<'id>) {
        if self.rule(r).status != Status::Failed {
            self.set_status(r, Status::Failed);
            // Need to inform child rules they are unready now
            let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                .flat_map(|o| self[*o].children.iter()).map(|c| *c).collect();
            // This is a separate loop to satisfy the borrow checker.
            // It is somewhat less efficient to store this, but such
            // is life.  I could have stored a HashSet rather than a
            // Vec, but suspect the hash table overhead would make it
            // less efficient.
            for childr in children.iter() {
                self.failed(*childr);
            }

            // Delete any files that were created, so that they will
            // be properly re-created next time this command is run.
            for &o in self.rule(r).all_outputs.iter() {
                if !self[o].is_in_git {
                    self[o].unlink();
                }
            }
        }
    }
    fn built(&mut self, r: RuleRef<'id>) {
        self.set_status(r, Status::Built);
        // Only check "unready" children to see if they might now be
        // ready to be built.  Other children might not be desired as
        // part of our build, either because they are non-default, or
        // because the user requested specific targets.
        let children: Vec<RuleRef<'id>> = self.rule(r).all_outputs.iter()
            .flat_map(|o| self[*o].children.iter()).map(|c| *c)
            .filter(|&c| self.rule(c).status == Status::Unready).collect();
        if children.len() > 0 {
            vvprintln!(    " ^^^ Have built {}, looking at children", self.pretty_rule(r));
            for childr in children {
                vvprintln!("     -> child: {}", self.pretty_rule(childr));
                self.check_cleanliness(childr);
            }
        }
    }

    fn summarize_build_results(&self) -> i32 {
        if self.statuses[Status::Failed].len() > 0 {
            println!("Build failed {}/{} failures",
                     self.statuses[Status::Failed].len(),
                     self.statuses[Status::Failed].len()
                     + self.statuses[Status::Built].len());
            return self.statuses[Status::Failed].len() as i32;
        }
        if let Some(err) = self.check_strictness() {
            println!("Build failed due to {}!", err);
            1
        } else {
            println!("Build succeeded!");
            0
        }
    }

    fn check_strictness(&self) -> Option<String> {
        match self.flags.strictness {
            flags::Strictness::Strict => {
                // Checking for strict dependencies is harder than for
                // exhaustive, because we need to ensure that
                // everything will be built prior to being needed.
                // For each input there must be an explicit input that
                // has the same rule that generates that input.
                println!("Checking for strict dependencies...");
                let mut fails = None;
                for r in self.rulerefs() {
                    for &i in self.rule(r).all_inputs.iter() {
                        if !self.rule(r).inputs.contains(&i) {
                            if self[i].rule.is_some() {
                                let mut have_rule = false;
                                for &j in self.rule(r).inputs.iter() {
                                    have_rule = have_rule || (self[j].rule == self[i].rule)
                                }
                                if !have_rule {
                                    println!("missing dependency: \"{}\" requires {}",
                                             self.pretty_rule(r),
                                             self.pretty_path(i).to_string_lossy());
                                    fails = Some(String::from("missing dependencies"));
                                }
                            }
                        }
                    }
                }
                fails
            },
            flags::Strictness::Exhaustive => {
                unimplemented!()
            },
            flags::Strictness::Normal => {
                None
            },
        }
    }

    /// Actually run a command.  This needs to update the inputs and
    /// outputs.  FIXME
    pub fn run(&mut self, r: RuleRef<'id>) -> io::Result<()> {
        {
            // Before running, let us fill in any hash info for
            // inputs that we have not yet read about.
            let v: Vec<FileRef<'id>> = self.rule(r).all_inputs.iter()
                .filter(|&w| self[*w].hashstat.unfinished()).map(|&w|w).collect();
            for w in v {
                let p = self[w].path.clone(); // ugly workaround for borrow checker
                self[w].hashstat.finish(&p).unwrap();
            }
        }
        let wd = self.flags.root.join(&self.rule(r).working_directory);
        let mut stat = if let Some(ref logdir) = self.flags.log_output {
            std::fs::create_dir_all(logdir)?;
            let d = self.flags.root.join(logdir).join(self.sanitize_rule(r));
            bigbro::Command::new("/bin/sh")
                .arg("-c")
                .arg(&self.rule(r).command)
                .current_dir(&wd)
                .stdin(bigbro::Stdio::null())
                .log_stdouterr(&d)
                .status()?
        } else {
            bigbro::Command::new("/bin/sh")
                .arg("-c")
                .arg(&self.rule(r).command)
                .current_dir(&wd)
                .stdin(bigbro::Stdio::null())
                .save_stdouterr()
                .status()?
        };

        let num_built = 1 + self.statuses[Status::Failed].len()
            + self.statuses[Status::Built].len();
        let num_total = self.statuses[Status::Failed].len()
            + self.statuses[Status::Built].len()
            + self.statuses[Status::Building].len()
            + self.statuses[Status::Dirty].len()
            + self.statuses[Status::Unready].len();
        let message: String;
        let abort = |sel: &mut Build<'id>, stat: &bigbro::Status, errmsg: &str| -> io::Result<()> {
            println!("error: {}", errmsg);
            sel.failed(r);
            // now remove the output of this rule
            for w in stat.written_to_files() {
                std::fs::remove_file(w).ok();
            }
            for d in stat.mkdir_directories() {
                std::fs::remove_dir_all(d).ok();
            }
            let ff = sel.rule(r).facfile;
            sel.facfiles_used.insert(ff);
            Ok(())
        };

        if stat.status().success() {
            // First clear out the listing of inputs and outputs
            self.rule_mut(r).all_inputs.clear();
            let mut old_outputs: HashSet<FileRef<'id>> =
                self.rule_mut(r).all_outputs.drain().collect();
            for w in stat.written_to_files() {
                if w.starts_with(&self.flags.root)
                    && !is_git_path(&w)
                    && !self.rule(r).is_cache(&w) {
                        let fw = self.new_file(&w);
                        if self[fw].hashstat.finish(&w).is_ok() {
                            if let Some(fwr) = self[fw].rule {
                                if fwr != r {
                                    let mess = format!("two rules generate same output {:?}:\n\t{}\nand\n\t{}",
                                                       self.pretty_path_peek(fw),
                                                       self.pretty_rule(r),
                                                       self.pretty_rule(fwr));
                                    return abort(self, &stat, &mess);
                                }
                            }
                            self.add_output(r, fw);
                            old_outputs.remove(&fw);
                        } else {
                            vprintln!("   Hash not okay?!");
                        }
                    }
            }
            for d in stat.mkdir_directories() {
                if d.starts_with(&self.flags.root)
                    && !is_git_path(&d)
                    && !self.rule(r).is_cache(&d)
                {
                    let fw = self.new_file(&d);
                    if self[fw].hashstat.finish(&d).is_ok() {
                        // We allow multiple rules to mkdir the same
                        // directory.  This is fine, since we do not
                        // apply strict ordering to the creation of a
                        // directory.
                        self.add_output(r, fw);
                        old_outputs.remove(&fw);
                    }
                }
            }
            for rr in stat.read_from_files() {
                if !is_boring(&rr) && !self.rule(r).is_cache(&rr) {
                    let fr = self.new_file(&rr);
                    if !old_outputs.contains(&fr)
                        && self[fr].hashstat.finish(&rr).is_ok()
                    {
                        let hs = self[fr].hashstat;
                        self.rule_mut(r).hashstats.insert(fr, hs);
                        self.add_input(r, fr);
                    }
                }
            }
            for rr in stat.read_from_directories() {
                if !is_boring(&rr) && !self.rule(r).is_cache(&rr) {
                    let fr = self.new_file(&rr);
                    if self[fr].hashstat.finish(&rr).is_ok() {
                        let hs = self[fr].hashstat;
                        self.rule_mut(r).hashstats.insert(fr, hs);
                        self.add_input(r, fr);
                    }
                }
            }
            // Here we add in any explicit inputs our outputs that
            // were not actually read or touched.
            let explicit_inputs = self.rule(r).inputs.clone();
            for i in explicit_inputs {
                if !self.rule(r).all_inputs.contains(&i) {
                    self.add_input(r, i);
                }
            }
            let mut failed_to_produce_output = false;
            let explicit_outputs = self.rule(r).outputs.clone();
            for o in explicit_outputs {
                if !self.rule(r).all_outputs.contains(&o) {
                    self.add_output(r, o);
                    if !self[o].exists() {
                        println!("build failed to create: {:?}",
                                 self.pretty_path_peek(o));
                        failed_to_produce_output = true;
                    }
                }
            }
            for o in old_outputs {
                // Any previously created files that still exist
                // should be treated as if they were created this time
                // around.
                if self[o].exists() {
                    self.add_output(r, o);
                }
            }
            {
                // let us fill in any hash info for outputs that were
                // not touched in this build.
                let v: Vec<FileRef<'id>> = self.rule(r).all_outputs.iter()
                    .filter(|&w| self[*w].hashstat.unfinished()).map(|&w|w).collect();
                for w in v {
                    let p = self[w].path.clone();
                    if self[w].hashstat.finish(&p).is_err() {
                        // presumably this file no longer exists, so
                        // we should remove it from the list of
                        // outputs.
                        self.rule_mut(r).all_outputs.remove(&w);
                    }
                }
            }
            if failed_to_produce_output {
                message = format!("build failed: {}", self.pretty_rule(r));
                println!("!{}/{}!: {}",
                         num_built, num_total, message);
                self.failed(r);
            } else {
                message = self.pretty_rule(r);
                println!("[{}/{}]: {}", num_built, num_total, &message);
                self.built(r);
            }
        } else {
            message = format!("build failed: {}", self.pretty_rule(r));
            println!("!{}/{}!: {}",
                     num_built, num_total, message);
            self.failed(r);
        }
        if self.flags.show_output || !stat.status().success() {
            let f = stat.stdout()?;
            let mut contents = String::new();
            f.unwrap().read_to_string(&mut contents)?;
            if contents.len() > 0 {
                println!("{}", contents);
                println!("end of output from: {}", &message);
            }
        }
        let ff = self.rule(r).facfile;
        self.facfiles_used.insert(ff);
        Ok(())
    }

    /// Formats the path nicely as a relative path if possible
    pub fn pretty_path(&self, p: FileRef<'id>) -> PathBuf {
        match self[p].path.strip_prefix(&self.flags.root) {
            Ok(p) => PathBuf::from(p),
            Err(_) => self[p].path.clone(),
        }
    }
    /// Formats the path nicely as a relative path if possible
    pub fn pretty_path_peek(&self, p: FileRef<'id>) -> &Path {
        match self[p].path.strip_prefix(&self.flags.root) {
            Ok(p) => p,
            Err(_) => &self[p].path,
        }
    }

    /// Formats the rule nicely if possible
    pub fn pretty_rule(&self, r: RuleRef<'id>) -> String {
        if self.rule(r).working_directory == self.flags.root {
            self.rule(r).command.to_string_lossy().into_owned()
        } else {
            format!("{}: {}",
                    self.rule(r).working_directory
                        .strip_prefix(&self.flags.root).unwrap().to_string_lossy(),
                    self.rule(r).command.to_string_lossy())
        }
    }

    /// pretty_reason is a way of describing a rule in terms of why it
    /// needs to be built.  If the rule is always built by default, it
    /// gives the same output that pretty_rule does.  However, if it
    /// is a non-default rule, it selects an output which is actually
    /// needed to describe why it needs to be built.
    pub fn pretty_reason(&self, r: RuleRef<'id>) -> String {
        if self.rule(r).is_default {
            return self.pretty_rule(r);
        }
        for &o in self.rule(r).outputs.iter() {
            for &c in self[o].children.iter() {
                if self.rule(c).status == Status::Unready
                    || self.rule(c).status == Status::Failed
                {
                    return self.pretty_path_peek(o).to_string_lossy().into_owned();
                }
            }
        }
        self.pretty_rule(r) // ?!
    }


    /// Formats the rule as a sane filename
    fn sanitize_rule(&self, r: RuleRef<'id>) -> PathBuf {
        let rr = if self.rule(r).outputs.len() == 1 {
            self.pretty_path(self.rule(r).outputs[0]).to_string_lossy().into_owned()
        } else {
            self.pretty_rule(r)
        };
        let mut buf = String::with_capacity(rr.len());
        for c in rr.chars() {
            match c {
                'a' ... 'z' | 'A' ... 'Z' | '0' ... '9' | '_' | '-' | '.' => buf.push(c),
                _ => buf.push('_'),
            }
        }
        PathBuf::from(buf)
    }

    /// Look up the rule
    pub fn rule_mut(&mut self, r: RuleRef<'id>) -> &mut Rule<'id> {
        &mut self.rules[r.0]
    }
    /// Look up the rule
    pub fn rule(&self, r: RuleRef<'id>) -> &Rule<'id> {
        &self.rules[r.0]
    }

}

impl<'id> std::ops::Index<FileRef<'id>> for Build<'id>  {
    type Output = File<'id>;
    fn index(&self, r: FileRef<'id>) -> &File<'id> {
        &self.files[r.0]
    }
}
impl<'id> std::ops::IndexMut<FileRef<'id>> for Build<'id>  {
    fn index_mut(&mut self, r: FileRef<'id>) -> &mut File<'id> {
        &mut self.files[r.0]
    }
}

#[cfg(unix)]
fn bytes_to_osstr(b: &[u8]) -> &OsStr {
    OsStr::from_bytes(b)
}

#[cfg(not(unix))]
fn bytes_to_osstr(b: &[u8]) -> &OsStr {
    Path::new(std::str::from_utf8(b).unwrap()).as_os_str()
}


/// This is a path in the git repository that we should ignore
pub fn is_git_path(path: &Path) -> bool {
    path.starts_with(".git") && !path.starts_with(".git/hooks")
}

/// This path is inherently boring
pub fn is_boring(path: &Path) -> bool {
    path.starts_with("/proc") || path.starts_with("/dev") || is_git_path(path)
}

fn normalize(p: &Path) -> PathBuf {
    let mut out = PathBuf::new();
    for element in p.iter() {
        if element == ".." {
            out.pop();
        } else {
            out.push(element);
        }
        if let Ok(o) = out.canonicalize() {
            out = o;
        }
    }
    out
}
