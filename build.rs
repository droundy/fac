extern crate gcc;
extern crate git2;

use std::os::unix::io::{FromRawFd, IntoRawFd};

fn main() {

    std::fs::remove_dir_all("bigbro").ok();
    git2::build::RepoBuilder::new()
        .clone("git://github.com/droundy/bigbro.git", std::path::Path::new("bigbro"))
        .unwrap();
    let linux_h_fd = std::fs::File::create("bigbro/syscalls/linux.h")
        .unwrap().into_raw_fd();
    std::process::Command::new("python3")
        .args(&["syscalls/linux.py"])
        .current_dir("bigbro")
        .stdout(unsafe {std::process::Stdio::from_raw_fd(linux_h_fd)})
        .status().unwrap();

    let version_fd = std::fs::File::create("version-identifier.h")
        .unwrap().into_raw_fd();
    std::process::Command::new("python")
        .args(&["generate-version-header.py"])
        .stdout(unsafe {std::process::Stdio::from_raw_fd(version_fd)})
        .status().unwrap();

    gcc::Config::new()
                .file("arguments.c")
                .file("build.c")
                .file("clean-all.c")
                .file("environ.c")
                .file("fac.c")
                .file("files.c")
                .file("git.c")
                .file("mkdir.c")
                .file("targets.c")
                .file("lib/iterablehash.c")
                .file("lib/listset.c")
                .file("lib/sha1.c")
                .file("bigbro/bigbro-linux.c")
                .include("bigbro")
                .compile("libfac.a");
}
