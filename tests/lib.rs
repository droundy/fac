extern crate fac;

use std::io::{Write, Read};

struct TempDir(std::path::PathBuf);
impl TempDir {
    fn new<P: AsRef<std::path::Path>> (p: P) -> TempDir {
        println!("remove test repository");
        std::fs::remove_dir_all(&p.as_ref()).ok();
        println!("create {:?}", p.as_ref());
        assert!(std::fs::create_dir_all(p.as_ref()).is_ok());
        TempDir(std::path::PathBuf::from(p.as_ref()))
    }
    fn fac(&self, args: &[&str]) -> std::process::Output {
        let newpath =
            match std::env::var_os("PATH") {
                Some(paths) => {
                    let mut new_paths = vec![location_of_executables()];
                    for path in std::env::split_paths(&paths) {
                        new_paths.push(path);
                    }
                    std::env::join_paths(new_paths).unwrap()
                }
                None => {
                    println!("PATH is not defined in the environment.");
                    std::env::join_paths(&[std::env::current_dir().unwrap()
                                           .join("target/debug")]).unwrap()
                },
            };
        println!("PATH is {:?}", &newpath);
        let s = std::process::Command::new("fac").args(args).env("PATH", newpath)
            .current_dir(&self.0).output();
        println!("I am in {:?} with args {:?}", std::env::current_dir(), args);
        if !s.is_ok() {
            println!("Bad news: {:?}", s);
            println!("  exists:: {:?}", std::path::Path::new("target/debug/fac").exists());
            for x in std::path::Path::new("target/debug").read_dir().unwrap() {
                println!("  target/debug has {:?}", x);
            }
        } else {
            let s = s.unwrap();
            println!("output is {:?}", String::from_utf8_lossy(&s.stdout));
            return s;
        }
        s.unwrap()
    }
    fn git_init(&self) {
        let s = std::process::Command::new("git").arg("init")
            .current_dir(&self.0).output().unwrap();
        assert!(s.status.success());
    }
    fn add_file(&self, p: &str, contents: &[u8]) {
        let absp = self.0.join(p);
        let mut f = std::fs::File::create(absp).unwrap();
        f.write(contents).unwrap();
        let s = std::process::Command::new("git").arg("add").arg(p)
            .current_dir(&self.0).output().unwrap();
        assert!(s.status.success());
    }
    fn expect_file(&self, p: &str, contents: &[u8]) {
        let absp = self.0.join(p);
        let mut f = std::fs::File::open(absp).unwrap();
        let mut actual_contents = Vec::new();
        f.read_to_end(&mut actual_contents).unwrap();
        assert_eq!(std::str::from_utf8(actual_contents.as_slice()),
                   std::str::from_utf8(contents));
    }
}
impl Drop for TempDir {
    fn drop(&mut self) {
        std::fs::remove_dir_all(&self.0).unwrap();
    }
}

/// This test is mostly to confirm that we are in fact testing the fac
/// that we just compiled!
#[test]
fn fac_version() {
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"
| fac --version > version
");
    assert!(tempdir.fac(&[]).status.success());
    tempdir.expect_file("version", format!("fac {}\n", fac::version::VERSION).as_bytes());
}

#[test]
fn echo_to_file() {
    assert!(fac::version::VERSION.len() > 0);
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"# comment
| echo hello world > foo
");
    tempdir.expect_file("top.fac", b"# comment
| echo hello world > foo
");
    assert!(tempdir.fac(&[]).status.success());
    tempdir.expect_file("foo", b"hello world\n");
}

#[test]
fn failing_rule() {
    assert!(fac::version::VERSION.len() > 0);
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"# comment
| ec ho hello world > foo
");
    assert!(! tempdir.fac(&[]).status.success());
}

fn location_of_executables() -> std::path::PathBuf {
    // The key here is that this test executable is located in almost
    // the same place as the built `fac` is located.
    let mut path = std::env::current_exe().unwrap();
    path.pop(); // chop off exe name
    path.pop(); // chop off "deps"
    path
}

