//! Support interaction with git

use std;

use std::ffi::OsString;
use std::os::unix::ffi::OsStringExt;

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
    let p = std::path::PathBuf::from(OsString::from_vec(output.stdout));
    std::env::set_current_dir(&p).unwrap();
    p
}

/// Find out what files are in git.
pub fn ls_files() -> std::collections::HashSet<std::path::PathBuf> {
    let output = std::process::Command::new("git")
        .arg("ls-files")
        .arg("-z")
        .output()
        .expect("Error calling git ls-files");
    let mut fs = std::collections::HashSet::new();
    for s in output.stdout.split(|c| *c == 0) {
        if s.len() > 0 {
            let p = std::path::PathBuf::from(OsString::from_vec(Vec::from(s)));
            fs.insert(p);
        }
    }
    fs
}
