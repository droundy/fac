//! Rules and types for building the stuff.

extern crate typed_arena;

use std;

use std::cell::{Cell, RefCell};
use refset::{RefSet};

use git;

/// The status of a rule.
#[derive(PartialEq, Eq, Hash, Copy, Clone)]
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

struct StatusMap<T>( [T;9] );

impl<T> StatusMap<T> {
    fn new<F>(v: F) -> StatusMap<T>
        where F: Fn() -> T
    {
        StatusMap( [v(),v(),v(),v(),v(),v(),v(),v(),v()] )
    }
}

impl<T> std::ops::Index<Status> for StatusMap<T>  {
    type Output = T;
    fn index(&self, s: Status) -> &T {
        &self.0[s as usize]
    }
}

/// Is the file a regular file, a symlink, or a directory?
#[derive(PartialEq, Eq, Hash, Copy, Clone)]
pub enum FileKind {
    /// It is a regular file
    File,
    /// It is a directory
    Dir,
    /// It is a symlink
    Symlink,
}

/// A file (or directory) that is either an input or an output for
/// some rule.
pub struct File<'a> {
    // build: &'a Build<'a>,
    rule: RefCell<Option<&'a Rule<'a>>>,
    path: std::path::PathBuf,
    // Question: could Vec be more efficient than RefSet here? It
    // depends if we add a rule multiple times to the same set of
    // children.  FIXME check this!
    children: RefCell<RefSet<'a, Rule<'a>>>,

    kind: Cell<Option<FileKind>>,
    is_in_git: bool,
}

impl<'a> File<'a> {
    /// Declare that this File is dirty (i.e. has been modified since
    /// the last build).
    pub fn dirty(&self) {
        for r in self.children.borrow().iter() {
            r.dirty();
        }
    }

    /// Set file properties...
    pub fn stat(&self) -> std::io::Result<FileKind> {
        let attr = std::fs::metadata(&self.path)?;
        self.kind.set(if attr.file_type().is_symlink() {
            Some(FileKind::Symlink)
        } else if attr.file_type().is_dir() {
            Some(FileKind::Dir)
        } else if attr.file_type().is_file() {
            Some(FileKind::File)
        } else {
            None
        });
        match self.kind.get() {
            Some(k) => Ok(k),
            None => Err(std::io::Error::new(std::io::ErrorKind::Other, "irregular file")),
        }
    }

    /// Is this `File` in git?
    pub fn in_git(&self) -> bool {
        self.is_in_git
    }
}

/// A rule for building something.
pub struct Rule<'a> {
    build: &'a Build<'a>,
    inputs: RefCell<Vec<&'a File<'a>>>,
    outputs: RefCell<Vec<&'a File<'a>>>,

    status: Cell<Status>,
}

impl<'a> Rule<'a> {
    /// Add a new File as an input to this rule.
    pub fn add_input<'b: 'a>(&'b self, input: &'a File<'a>) -> &Rule<'a> {
        self.inputs.borrow_mut().push(input);
        input.children.borrow_mut().insert(self);
        self
    }
    /// Add a new File as an output of this rule.
    pub fn add_output<'b: 'a>(&'b self, input: &'a File<'a>) -> &Rule<'a> {
        self.outputs.borrow_mut().push(input);
        *input.rule.borrow_mut() = Some(self);
        self
    }
    /// Adjust the status of this rule, making sure to keep our sets
    /// up to date.
    pub fn set_status<'b: 'a>(&'b self, s: Status) {
        self.build.statuses[self.status.get()].borrow_mut().remove(self);
        self.build.statuses[s].borrow_mut().insert(self);
        self.status.set(s);
    }
    /// Mark this rule as dirty, adjusting other rules to match.
    pub fn dirty<'b: 'a>(&'b self) {
        let oldstat = self.status.get();
        if oldstat != Status::Dirty {
            self.set_status(Status::Dirty);
            if oldstat != Status::Unready {
                // Need to inform child rules they are unready now
                for o in self.outputs.borrow().iter() {
                    for r in o.children.borrow().iter() {
                        r.unready();
                    }
                }
            }
        }
    }
    /// Make this rule (and any that depend on it) `Status::Unready`.
    pub fn unready<'b: 'a>(&'b self) {
        if self.status.get() != Status::Unready {
            self.set_status(Status::Unready);
            // Need to inform child rules they are unready now
            for o in self.outputs.borrow().iter() {
                for r in o.children.borrow().iter() {
                    r.unready();
                }
            }
        }
    }
}


/// A struct that holds all the information needed to build.  You can
/// think of this as behaving like a set of global variables, but we
/// can drop the whole thing.  It is implmented using arena
/// allocation, so all of our Rules and Files are guaranteed to live
/// as long as the Build lives.
pub struct Build<'a> {
    alloc_files: typed_arena::Arena<File<'a>>,
    alloc_rules: typed_arena::Arena<Rule<'a>>,
    files: RefCell<std::collections::HashMap<&'a std::path::Path, &'a File<'a>>>,
    rules: RefCell<RefSet<'a, Rule<'a>>>,

    statuses: StatusMap<RefCell<RefSet<'a, Rule<'a>>>>,
}

impl<'a> Build<'a> {
    /// Construct a new `Build`.
    pub fn new() -> Build<'a> {
        let b: Build<'a> = Build {
            alloc_files: typed_arena::Arena::new(),
            alloc_rules: typed_arena::Arena::new(),
            files: RefCell::new(std::collections::HashMap::new()),
            rules: RefCell::new(RefSet::new()),

            statuses: StatusMap::new(|| RefCell::new(RefSet::new())),
        };
        for ref f in git::ls_files() {
            println!("i see {:?}", f);
            // fixme: the following line causes trouble, "does not
            // live long enough".

            // b.new_file_private(f, true);
        }
        b
    }

    /// Allocate space for a new `Rule`.
    pub fn new_rule<'b: 'a>(&'b self) -> &'a Rule<'a> {
        let r = self.alloc_rules.alloc(Rule {
            inputs: RefCell::new(vec![]),
            outputs: RefCell::new(vec![]),
            status: Cell::new(Status::Unknown),
            build: self,
        });
        self.statuses[Status::Unknown].borrow_mut().insert(r);
        self.rules.borrow_mut().insert(r);
        r
    }

    /// Look up a `File` corresponding to a path, or if it doesn't
    /// exist, allocate space for a new `File`.
    ///
    /// # Examples
    ///
    /// ```
    /// use fac::build;
    /// let mut b = build::Build::new();
    /// let t = b.new_file("test");
    /// ```
    pub fn new_file<'b: 'a , P: AsRef<std::path::Path>>(&'b self, path: P) -> &'a File<'a> {
        self.new_file_private(path, false)
    }

    fn new_file_private<'b: 'a, P: AsRef<std::path::Path>>(&'b self, path: P,
                                                           is_in_git: bool)
                                                           -> &'a File<'a> {
        // If this file is already in our database, use the version
        // that we have.  It is an important invariant that we can
        // only have one file with a given path in the database.
        match self.files.borrow().get(path.as_ref()) {
            Some(f) => return f,
            None => ()
        }
        let f = self.alloc_files.alloc(File {
            // build: self,
            rule: RefCell::new(None),
            path: std::path::PathBuf::from(path.as_ref()),
            children: RefCell::new(RefSet::new()),
            kind: Cell::new(None),
            is_in_git: is_in_git,
        });
        self.files.borrow_mut().insert(& f.path, f);
        f
    }
}
