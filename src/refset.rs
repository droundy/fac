use std;
use std::hash::{Hash, Hasher};

struct HashableRef<'a, T: 'a> (&'a T);

impl<'a, T: 'a> Hash for HashableRef<'a, T> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        (self.0 as *const T).hash(state);
    }
}

impl<'a, T: 'a> std::cmp::PartialEq for HashableRef<'a, T> {
    fn eq(&self, other: &HashableRef<'a,T>) -> bool {
        (self.0 as *const T) == (other.0 as *const T)
    }
}
impl<'a, T: 'a> Eq for HashableRef<'a, T> {}

pub struct RefSet<'a, T: 'a> (std::collections::HashSet<HashableRef<'a,T>>);

fn unwrapref<'a,T:'a>(x: &HashableRef<'a,T>) -> &'a T {
    x.0
}
fn unwrapref2<'a,T:'a>(x: HashableRef<'a,T>) -> &'a T {
    x.0
}

impl<'a, T: 'a> RefSet<'a, T> {
    pub fn new() -> RefSet<'a,T> {
        RefSet(std::collections::HashSet::new())
    }

    pub fn with_capacity(sz: usize) -> RefSet<'a,T> {
        RefSet(std::collections::HashSet::with_capacity(sz))
    }

    pub fn iter(&self)
                -> std::iter::Map<std::collections::hash_set::Iter<HashableRef<'a,T>>,
                                  fn(&HashableRef<'a,T>) -> &'a T>
    {
        self.0.iter().map(unwrapref)
    }

    pub fn len(&self) -> usize { self.0.len() }

    pub fn drain(&mut self)
                 -> std::iter::Map<std::collections::hash_set::Drain<HashableRef<'a,T>>,
                                   fn(HashableRef<'a,T>) -> &'a T>
    {
        self.0.drain().map(unwrapref2)
    }

    pub fn clear(&mut self) { self.0.clear() }

    pub fn contains(&self, value: &'a T) -> bool {
        self.0.contains(&HashableRef(value))
    }

    pub fn insert(&mut self, value: &'a T) -> bool {
        self.0.insert(HashableRef(value))
    }

    pub fn remove(&mut self, value: &'a T) -> bool {
        self.0.remove(&HashableRef(value))
    }
}
