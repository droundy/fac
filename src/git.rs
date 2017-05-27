//! Support interaction with git

use std;

/// Go to the top level of the git repository (typically the one
/// containing a `.git` directory).
pub fn go_to_top() -> std::path::PathBuf {
    let mut output = std::process::Command::new("git")
        .args(&["rev-parse", "--show-toplevel"])
        .output()
        .expect("Error calling git rev-parse --show-toplevel");
// #ifdef _WIN32
//   if (strlen(buf) > 2 && buf[0] == '/' && buf[2] == '/') {
// 	  // this is a workaround for a broken git included in msys2
// 	  // which returns paths like /c/Users/username...
// 	  buf[0] = buf[1];
// 	  buf[1] = ':';
//   }
// #endif

    let newlen = output.stdout.len()-1;
    output.stdout.truncate(newlen);
    let p = bytes_to_path(&output.stdout);
    std::env::set_current_dir(&p).unwrap();
    p
}

/// Location of the .git directory
pub fn git_dir() -> std::path::PathBuf {
    let mut output = std::process::Command::new("git")
        .args(&["rev-parse", "--git-dir"])
        .output()
        .expect("Error calling git rev-parse --git-dir");
// #ifdef _WIN32
//   if (strlen(buf) > 2 && buf[0] == '/' && buf[2] == '/') {
// 	  // this is a workaround for a broken git included in msys2
// 	  // which returns paths like /c/Users/username...
// 	  buf[0] = buf[1];
// 	  buf[1] = ':';
//   }
// #endif

    let newlen = output.stdout.len()-1;
    output.stdout.truncate(newlen);
    bytes_to_path(&output.stdout)
}

/// Find out what files are in git.
pub fn ls_files() -> std::collections::HashSet<std::path::PathBuf> {
    let output = std::process::Command::new("git")
        .arg("ls-files")
        .arg("-z")
        .output()
        .expect("Error calling git ls-files");
    let mut fs = std::collections::HashSet::new();
    for s in output.stdout.split(|c| *c == b'\0') {
        if s.len() > 0 {
            fs.insert(bytes_to_path(s));
        }
    }
    fs
}

/// git add a file or more
pub fn add(p: &std::path::Path) -> std::io::Result<()> {
    let s = std::process::Command::new("git")
        .arg("add")
        .arg(p)
        .output()?;
    if s.status.success() {
        Ok(())
    } else {
        Err(std::io::Error::new(std::io::ErrorKind::Other,
                                String::from_utf8_lossy(&s.stderr).into_owned()))
    }
}

#[test]
fn ls_files_works() {
    let x = ls_files();
    assert!(x.contains(&bytes_to_path(b".gitignore")));
}

#[cfg(unix)]
use std::os::unix::ffi::OsStrExt;

#[cfg(unix)]
fn bytes_to_path(b: &[u8]) -> std::path::PathBuf {
    std::path::PathBuf::from(std::path::Path::new(std::ffi::OsStr::from_bytes(b)))
}

#[cfg(not(unix))]
fn bytes_to_path(b: &[u8]) -> std::path::PathBuf {
    std::path::PathBuf::from(std::path::Path::new(std::str::from_utf8(b).unwrap()))
}

