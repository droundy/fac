//! Rules and types for building the stuff.

#[cfg(test)]
extern crate quickcheck;

extern crate typed_arena;
extern crate bigbro;

use std;

use std::ffi::{OsString, OsStr};
use std::path::{Path,PathBuf};

use std::collections::{HashSet, HashMap};

use std::io::{Read};

use git;

pub mod hashstat;
pub mod flags;

/// VERBOSE is used to enable our vprintln macro to know the
/// verbosity.  This is a bit ugly, but is needed due to rust macros
/// being hygienic.
static mut VERBOSE: bool = false;

/// The `vprintln!` macro does a println! only if the --verbose flag
/// is specified.  It is written as a macro because if it were a
/// method or function then the arguments would be always evaluated
/// regardless of the verbosity (thus slowing things down).
/// Equivalently, we could write an if statement for each verbose
/// print, but that would be tedious.
macro_rules! vprintln {
    () => {{ if unsafe { VERBOSE } { println!() } }};
    ($fmt:expr) => {{ if unsafe { VERBOSE } { println!($fmt) } }};
    ($fmt:expr, $($arg:tt)*) => {{ if unsafe { VERBOSE } { println!($fmt, $($arg)*) } }};
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

    rules_defined: HashSet<RuleRef<'id>>,

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
    pub fn stat(&mut self) -> std::io::Result<FileKind> {
        self.hashstat = hashstat::stat(&self.path)?;
        match self.hashstat.kind {
            Some(k) => Ok(k),
            None => Err(std::io::Error::new(std::io::ErrorKind::Other, "irregular file")),
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
    path.as_os_str().as_bytes()[pp-l..] == suff.as_bytes()[..]
}
#[cfg(unix)]
fn is_prefix(path: &Path, suff: &OsStr) -> bool {
    let l = suff.as_bytes().len();
    let p = path.as_os_str().as_bytes();
    p[p.len()-l..] == suff.as_bytes()[..]
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
    filerefs: Vec<FileRef<'id>>,
    rulerefs: Vec<RuleRef<'id>>,
    filemap: HashMap<PathBuf, FileRef<'id>>,
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
    unsafe { VERBOSE = fl.verbose; }
    // This approach to type witnesses is taken from
    // https://github.com/bluss/indexing/blob/master/src/container.rs
    let mut b = Build {
        id: Id::default(),
        files: Vec::new(),
        rules: Vec::new(),
        filerefs: Vec::new(),
        rulerefs: Vec::new(),
        filemap: HashMap::new(),
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
    pub fn build(&mut self) {
        for f in self.filerefs() {
            if self[f].is_fac_file() && self[f].rules_defined.len() == 0 {
                self.read_file(f).unwrap();
                self.print_fac_file(f).unwrap();
            }
        }
        self.mark_fac_files();
        let rules: Vec<_> = self.statuses[Status::Marked].iter().map(|&r| r).collect();
        for r in rules {
            self.check_cleanliness(r);
            vprintln!("I should see about: {}", self.pretty_rule(r));
        }
        let rules: Vec<_> = self.statuses[Status::Dirty].iter().map(|&r| r).collect();
        for r in rules {
            vprintln!("I shall run: {}", self.pretty_rule(r));
            if let Err(e) = self.run(r) {
                println!("I got err {}", e);
            }
        }
    }
    fn filerefs(&self) -> Vec<FileRef<'id>> {
        let mut out = Vec::new();
        for i in 0 .. self.files.len() {
            out.push(FileRef(i, self.id));
        }
        out
    }
    fn new_file_private<P: AsRef<Path>>(&mut self, path: P,
                                        is_in_git: bool)
                                        -> FileRef<'id> {
        // If this file is already in our database, use the version
        // that we have.  It is an important invariant that we can
        // only have one file with a given path in the database.
        match self.filemap.get(path.as_ref()) {
            Some(f) => return *f,
            None => ()
        }
        let f = FileRef(self.files.len(), self.id);
        self.files.push(File {
            id: self.id,
            rule: None,
            path: PathBuf::from(path.as_ref()),
            children: HashSet::new(),
            rules_defined: HashSet::new(),
            hashstat: hashstat::HashStat::empty(),
            is_in_git: is_in_git,
        });
        self.filemap.insert(PathBuf::from(path.as_ref()), f);
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
    pub fn read_file(&mut self, fileref: FileRef<'id>) -> std::io::Result<()> {
        let filepath = self[fileref].path.clone();
        let mut f = std::fs::File::open(&filepath)?;
        let mut v = Vec::new();
        f.read_to_end(&mut v)?;
        let mut command: Option<RuleRef<'id>> = None;
        for (lineno_minus_one, line) in v.split(|c| *c == b'\n').enumerate() {
            let lineno = lineno_minus_one + 1;
            fn parse_error<T>(path: &Path, lineno: usize, msg: &str) -> std::io::Result<T> {
                Err(std::io::Error::new(std::io::ErrorKind::Other,
                                        format!("error: {:?}:{}: {}",
                                                path, lineno, msg)))
            }
            if line.len() < 2 || line[0] == b'#' { continue };
            if line[1] != b' ' {
                return parse_error(&filepath, lineno,
                                   "Second character of line should be a space.");
            }
            let get_rule = |r: Option<RuleRef<'id>>, c: char| -> std::io::Result<RuleRef<'id>> {
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
                    self[fileref].rules_defined.insert(r);
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
                    let f = self.new_file(bytes_to_osstr(&line[2..]));
                    self.add_explicit_output(get_rule(command, '>')?, f);
                },
                b'<' => {
                    let f = self.new_file(bytes_to_osstr(&line[2..]));
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
        Ok(())
    }

    /// Write a fac file
    pub fn print_fac_file(&mut self, fileref: FileRef<'id>) -> std::io::Result<()> {
        for &r in self[fileref].rules_defined.iter() {
            println!("| {}", self.rule(r).command.to_string_lossy());
            for &i in self.rule(r).inputs.iter() {
                println!("< {}", self[i].path.to_string_lossy());
            }
            for &o in self.rule(r).outputs.iter() {
                println!("> {}", self[o].path.to_string_lossy());
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
        self.rule_mut(r).all_inputs.insert(input);
        self[input].children.insert(r);
    }
    /// Add a new File as an explicit output of this rule.
    fn add_explicit_output(&mut self, r: RuleRef<'id>, output: FileRef<'id>) {
        self.rule_mut(r).outputs.push(output);
        self.rule_mut(r).all_outputs.insert(output);
        self[output].rule = Some(r);
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

    fn check_cleanliness(&mut self, r: RuleRef<'id>) {
        let old_status = self.rule(r).status;
        if old_status != Status::Unknown && old_status != Status::Unready &&
            old_status != Status::Marked {
                return; // We already know if it is clean!
            }
        if self.rule(r).inputs.len() == 0 && self.rule(r).outputs.len() == 0 {
            // Presumably this means we have never built this rule, and its
            // inputs are in git.
            self.set_status(r, Status::Unready); // FIXME should sort by latency...
        }
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
                    self.set_status(r, Status::Unready);
                    return;
                }
        }
        for &i in r_other_inputs.iter() {
            if !self[i].in_git() &&
                self[i].rule.is_none() &&
                self[i].path.starts_with(&self.flags.root) &&
                !is_git_path(self.pretty_path_peek(i)) {
                    // One of our explicit inputs is not in git, and
                    // we also do not know how to build it.  But it
                    // was previously present and built using this
                    // input.  This should be rebuilt since it may
                    // have "depended" on that input via something
                    // like "cat *.input > foo", and now that it
                    // doesn't exist we must rebuild.
                    is_dirty = true;
                }
        }
        if !is_dirty {
            for &i in r_all_inputs.iter() {
                if let Some(istat) = self.rule(r).hashstats.get(&i).map(|s| *s) {
                    let path = self[i].path.clone();
                    if let Some(irule) = self[i].rule {
                        if self.rule(irule).status == Status::Built {
                            println!("Input was rebuilt, but was it changed?");
                            println!("FIXME this probably shouldn't be dealt with here...");
                            self[i].hashstat.finish(&path);
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

                } else {
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
            self.set_status(r, Status::Clean);
            if old_status == Status::Unready {
                // If we were previously unready, let us now check if
                // any of our children are ready.  FIXME: check if
                // this could end up resulting in building things we
                // didn't want built!
                let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                    .flat_map(|o| self[*o].children.iter()).map(|c| *c).collect();
                for childr in children {
                    self.check_cleanliness(childr);
                }
            }
        }
    }

    /// Mark this rule as dirty, adjusting other rules to match.
    pub fn dirty(&mut self, r: RuleRef<'id>) {
        let oldstat = self.rule(r).status;
        if oldstat != Status::Dirty {
            self.set_status(r, Status::Dirty);
            if oldstat != Status::Unready {
                // Need to inform child rules they are unready now
                let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                    .flat_map(|o| self[*o].children.iter()).map(|c| *c).collect();
                // This is a separate loop to satisfy the borrow checker.
                for childr in children.iter() {
                    self.unready(*childr);
                }
            }
        }
    }
    /// Make this rule (and any that depend on it) `Status::Unready`.
    pub fn unready(&mut self, r: RuleRef<'id>) {
        if self.rule(r).status != Status::Unready {
            self.set_status(r, Status::Unready);
            // Need to inform child rules they are unready now
            let children: Vec<RuleRef<'id>> = self.rule(r).outputs.iter()
                .flat_map(|o| self[*o].children.iter()).map(|c| *c).collect();
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

    /// Actually run a command.  This needs to update the inputs and
    /// outputs.  FIXME
    pub fn run(&mut self, r: RuleRef<'id>) -> std::io::Result<()> {
        let wd = self.flags.root.join(&self.rule(r).working_directory);
        vprintln!("running {:?} in {:?}", &self.rule(r).command, &wd,);
        bigbro::Command::new("/bin/sh").arg("-c")
            .arg(&self.rule(r).command)
            .current_dir(&wd)
            .status()?;
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
        self.rule(r).command.to_string_lossy().into_owned()
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
