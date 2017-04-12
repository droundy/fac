extern crate typed_arena;

use std;

use std::cell::{Cell, RefCell};
use refset::{RefSet};

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

pub struct File<'a> {
    build: &'a Build<'a>,
    rule: RefCell<Option<&'a Rule<'a>>>,
    path: std::path::PathBuf,
    // Question: could Vec be more efficient than RefSet here? It
    // depends if we add a rule multiple times to the same set of
    // children.  FIXME check this!
    children: RefCell<RefSet<'a, Rule<'a>>>,
}

impl<'a> File<'a> {
    pub fn dirty(& self) -> & File<'a> {
        for r in self.children.borrow().iter() {
            r.dirty();
        }
        self
    }
}

pub struct Rule<'a> {
    build: &'a Build<'a>,
    inputs: RefCell<Vec<&'a File<'a>>>,
    outputs: RefCell<Vec<&'a File<'a>>>,

    status: Cell<Status>,
}

impl<'a> Rule<'a> {
    pub fn add_input(&'a self, input: &'a File<'a>) -> &Rule<'a> {
        self.inputs.borrow_mut().push(input);
        input.children.borrow_mut().insert(self);
        self
    }
    pub fn add_output(&'a self, input: &'a File<'a>) -> &Rule<'a> {
        self.outputs.borrow_mut().push(input);
        *input.rule.borrow_mut() = Some(self);
        self
    }

    pub fn set_status(&'a self, s: Status) {
        self.build.statuses[self.status.get()].borrow_mut().remove(self);
        self.build.statuses[s].borrow_mut().insert(self);
        self.status.set(s);
    }
    pub fn dirty(&'a self) {
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
    pub fn unready(&'a self) {
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
            children: RefCell::new(RefSet::new()),
        });
        self.files.borrow_mut().insert(& f.path, f);
        f
    }
}
