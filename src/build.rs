extern crate typed_arena;

use std;

struct File<'a> {
    rule: std::cell::RefCell<Option<&'a Rule<'a>>>,
    path: std::path::PathBuf,
}

impl<'a> File<'a> {
    pub fn new(path: & std::path::Path) -> File<'a> {
        File {
            rule: std::cell::RefCell::new(None),
            path: std::path::PathBuf::from(path),
        }
    }
    pub fn set_rule(& self, r: &'a Rule<'a>) -> & File<'a> {
        *self.rule.borrow_mut() = Some(r);
        self
    }
}

struct Rule<'a> {
    inputs: std::cell::RefCell<Vec<&'a File<'a>>>,
    outputs: std::cell::RefCell<Vec<&'a File<'a>>>,
}

impl<'a> Rule<'a> {
    pub fn new() -> Rule<'a> {
        Rule {
            inputs: std::cell::RefCell::new(vec![]),
            outputs: std::cell::RefCell::new(vec![]),
        }
    }
    pub fn add_input(&self, input: &'a File<'a>) -> &Rule<'a> {
        self.inputs.borrow_mut().push(input);
        self
    }
}

struct Build<'a> {
    alloc_files: typed_arena::Arena<File<'a>>,
    alloc_rules: typed_arena::Arena<Rule<'a>>,
    files: std::collections::HashMap<String, &'a File<'a>>,
    rules: std::collections::HashMap<String, &'a Rule<'a>>,
}
