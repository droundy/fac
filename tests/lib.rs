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
            println!("output is:\n{}", String::from_utf8_lossy(&s.stdout));
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
        while b" \n\r".contains(&actual_contents[actual_contents.len()-1]) {
            actual_contents.pop();
        }
        let mut contents = Vec::from(contents);
        while b" \n\r".contains(&contents[contents.len()-1]) {
            contents.pop();
        }
        assert_eq!(std::str::from_utf8(actual_contents.as_slice()),
                   std::str::from_utf8(&contents));
    }
    fn no_such_file(&self, p: &str) {
        let absp = self.0.join(p);
        assert!(!absp.exists());
    }
}
impl Drop for TempDir {
    fn drop(&mut self) {
        std::fs::remove_dir_all(&self.0).ok(); // ignore errors that might happen on windows
    }
}

fn location_of_executables() -> std::path::PathBuf {
    // The key here is that this test executable is located in almost
    // the same place as the built `fac` is located.
    let mut path = std::env::current_exe().unwrap();
    path.pop(); // chop off exe name
    path.pop(); // chop off "deps"
    path
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
    tempdir.expect_file("version", format!("fac {}", fac::version::VERSION).as_bytes());
}

#[test]
fn echo_to_file() {
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"# comment
| echo hello world > foo
");
    tempdir.expect_file("top.fac", b"# comment
| echo hello world > foo");
    assert!(tempdir.fac(&[]).status.success());
    tempdir.expect_file("foo", b"hello world");
}

#[test]
fn dry_run() {
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"# comment
| echo hello world > foo
");
    tempdir.expect_file("top.fac", b"# comment
| echo hello world > foo
");
    let output = tempdir.fac(&["--dry"]);
    println!("output:\n{}\n", String::from_utf8_lossy(&output.stdout));
    assert!(output.status.success());
    assert!(has_match(&output.stdout, b"dry run"));
    assert!(has_match(&output.stdout, b"echo hello world > foo"));
    assert!(has_match(&output.stdout, b"Build failed"));
    tempdir.no_such_file("foo");
}

fn has_match(bigstr: &[u8], substr: &[u8]) -> bool {
    bigstr.windows(substr.len()).any(|x| x == substr)
}

#[test]
fn dependency_makefile() {
    if let Some(cc) = pick_executable(&["gcc", "cc", "clang", "cl.exe"]) {
        let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
        tempdir.git_init();
        tempdir.add_file("top.fac", format!("# comment
| {} -MD -MF .foo.o.dep -c foo.c
M .foo.o.dep

| {} -o foo foo.o
< foo.o
> foo

| ./foo > message
< foo
> message
", cc, cc).as_bytes());
        tempdir.add_file("foo.c", b"
#include <stdio.h>
#include \"foo.h\"

int main() {
  printf(message);
  return 0;
}
");
        tempdir.add_file("foo.h", b"
const char *message = \"hello\\n\";
");
        assert!(tempdir.fac(&["--blind"]).status.success());
        tempdir.expect_file("message", b"hello");
        assert!(tempdir.fac(&["--clean"]).status.success());
        tempdir.no_such_file("foo.o");
        tempdir.no_such_file(".foo.o.dep");
        tempdir.no_such_file("foo");
        tempdir.no_such_file("message");
    }
}

#[test]
fn failing_rule() {
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"# comment
| ec ho hello world > foo
");
    assert!(! tempdir.fac(&[]).status.success());
}

#[test]
fn failure_stdout_captured() {
    let tempdir = TempDir::new(&format!("tests/test-repositories/test-{}", line!()));
    tempdir.git_init();
    tempdir.add_file("top.fac", b"# comment
| echo this fails && false
");
    let output = tempdir.fac(&[]);
    assert!(! output.status.success());
    assert!(has_match(&output.stdout, b"this fails"));
}

fn executable_exists(cmd: &str) -> bool {
    std::process::Command::new(cmd).args(&["--missing-flag-hopefully"]).output().is_ok()
}

fn pick_executable(cmds: &[&'static str]) -> Option<&'static str> {
    cmds.iter().map(|&x| x).filter(|&x| executable_exists(x)).next()
}
