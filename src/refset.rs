//! A nice type for holding sets of references defined by address.
//! This can only really be used with either `'static` or with arena
//! allocators or the like.  But in those case, a `RefSet` ought to be
//! your most efficient way to store a set of items, provided you can
//! ensure that you don't ever allocate a duplicate.

use std;
use std::hash::{Hash, Hasher};

struct HashableRef<'a, T: 'a + ?Sized> (&'a T);

impl<'a, T: 'a + ?Sized> Hash for HashableRef<'a, T> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        // the following double cast is apparently needed to satisfy
        // rust, which for some reason will refuse to hash a pointer
        // to a ?Sized type.
        (self.0 as *const T as *const usize as usize).hash(state);
    }
}

impl<'a, T: 'a + ?Sized> std::cmp::PartialEq for HashableRef<'a, T> {
    fn eq(&self, other: &HashableRef<'a,T>) -> bool {
        (self.0 as *const T) == (other.0 as *const T)
    }
}
impl<'a, T: 'a + ?Sized> Eq for HashableRef<'a, T> {}

/// A set of items held by reference.
///
/// # Examples
///
/// ```
/// use fac::refset::RefSet;
/// let mut a = RefSet::new(); // this will have 'static lifetime
/// a.insert("Hello");
/// a.insert("World");
/// for s in a.iter() {
///     println!("{}", s)
/// }
/// // the following only passes because rust allocates each string only once
/// assert!(a.contains("Hello"));
/// assert!(a.contains("World"));
/// ```
pub struct RefSet<'a, T: 'a + ?Sized> (std::collections::HashSet<HashableRef<'a,T>>);

fn unwrapref<'a,T:'a + ?Sized>(x: &HashableRef<'a,T>) -> &'a T {
    x.0
}

/// The iterator for RefSet.
pub struct Iter<'b, 'a: 'b, T: 'a + ?Sized>(std::iter::Map<std::collections::hash_set::Iter<'b, HashableRef<'a,T>>,
                                              fn(&HashableRef<'a,T>) -> &'a T>);
impl<'b,'a: 'b,T: ?Sized> Iterator for Iter<'b,'a,T> {
    type Item = &'a T;
    fn next(& mut self) -> Option<&'a T> { self.0.next() }
    fn size_hint(& self) -> (usize, Option<usize>) { self.0.size_hint() }
}

impl<'a, T: 'a + ?Sized> RefSet<'a, T> {
    /// Create an empty set.
    pub fn new() -> RefSet<'a,T> {
        RefSet(std::collections::HashSet::new())
    }

    /// Create an empty set sized for given number.
    pub fn with_capacity(sz: usize) -> RefSet<'a,T> {
        RefSet(std::collections::HashSet::with_capacity(sz))
    }

    /// Iterate over the elements of the set.
    pub fn iter<'b>(&'b self) -> Iter<'b, 'a, T> {
        Iter(self.0.iter().map(unwrapref))
    }

    /// how many elements?
    pub fn len(&self) -> usize { self.0.len() }

    /// empty out the set
    pub fn clear(&mut self) { self.0.clear() }

    /// Is it in the set?
    pub fn contains(&self, value: &'a T) -> bool {
        self.0.contains(&HashableRef(value))
    }

    /// Add an element.
    pub fn insert(&mut self, value: &'a T) -> bool {
        self.0.insert(HashableRef(value))
    }

    /// Remove the element from the set.
    pub fn remove(&mut self, value: &'a T) -> bool {
        self.0.remove(&HashableRef(value))
    }
}
