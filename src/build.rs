extern crate typed_arena;

use std;

use std::cell::{Cell, RefCell};
use refset::{RefSet};

pub struct File<'a> {
    build: &'a Build<'a>,
    rule: RefCell<Option<&'a Rule<'a>>>,
    path: std::path::PathBuf,
}

impl<'a> File<'a> {
    // pub fn new(path: & std::path::Path) -> File<'a> {
    //     File {
    //         rule: RefCell::new(None),
    //         path: std::path::PathBuf::from(path),
    //     }
    // }
    pub fn set_rule(& self, r: &'a Rule<'a>) -> & File<'a> {
        *self.rule.borrow_mut() = Some(r);
        self
    }
}

#[derive(PartialEq, Eq, Hash, Copy, Clone)]
pub enum Status {
    Unknown,
    BeingDetermined,
    Clean,
    Built,
    Building,
    Failed,
    Marked, // means unknown, but want to build it
    Unready, // means that it is dirty but we cannot yet build it
    Dirty,
}

pub struct StatusMap<T>( [T;9] );

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

pub struct Rule<'a> {
    build: &'a Build<'a>,
    inputs: RefCell<Vec<&'a File<'a>>>,
    outputs: RefCell<Vec<&'a File<'a>>>,

    status: Cell<Status>,
}

impl<'a> Rule<'a> {
    pub fn add_input(&self, input: &'a File<'a>) -> &Rule<'a> {
        self.inputs.borrow_mut().push(input);
        self
    }
    pub fn add_output(&self, input: &'a File<'a>) -> &Rule<'a> {
        self.outputs.borrow_mut().push(input);
        self
    }

    pub fn set_status(&'a self, s: Status) {
        self.build.statuses[self.status.get()].borrow_mut().remove(self);
        self.build.statuses[s].borrow_mut().insert(self);
        self.status.set(s);
    }
}


pub struct Build<'a> {
    alloc_files: typed_arena::Arena<File<'a>>,
    alloc_rules: typed_arena::Arena<Rule<'a>>,
    files: RefCell<std::collections::HashMap<&'a std::path::Path, &'a File<'a>>>,
    rules: RefCell<RefSet<'a, Rule<'a>>>,

    statuses: StatusMap<RefCell<RefSet<'a, Rule<'a>>>>,
}

impl<'a> Build<'a> {
    pub fn new() -> Build<'a> {
        Build {
            alloc_files: typed_arena::Arena::new(),
            alloc_rules: typed_arena::Arena::new(),
            files: RefCell::new(std::collections::HashMap::new()),
            rules: RefCell::new(RefSet::new()),

            statuses: StatusMap::new(|| RefCell::new(RefSet::new())),
        }
    }

    pub fn new_rule(&'a self) -> &'a Rule<'a> {
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

    pub fn new_file(&'a self, path: & std::path::Path) -> &'a File<'a> {
        let f = self.alloc_files.alloc(File {
            build: self,
            rule: RefCell::new(None),
            path: std::path::PathBuf::from(path),
        });
        self.files.borrow_mut().insert(& f.path, f);
        f
    }
}
