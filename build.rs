
use std::process::Command;
use std::io::{Write};

fn main() {
    std::fs::remove_file("src/version.rs").ok();
    let mut stat = Command::new("git")
        .args(&["describe", "--dirty"])
        .output().unwrap();
    if !stat.status.success() {
        stat = Command::new("git")
            .args(&["revparse", "HEAD"])
            .output().unwrap();
    }

    let mut version = stat.stdout;
    let newlen = version.len()-1;
    version.truncate(newlen);

    let mut file = std::fs::File::create("src/version.rs").expect("error creating version.rs");
    file.write(b"/// The version of fac\npub static VERSION: &'static str = \"")
        .expect("error writing to file");
    file.write(&version).expect("error writing to file");
    file.write(b"\";\n").expect("error writing to file");
}
