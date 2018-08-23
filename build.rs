
use std::process::Command;
use std::io::{Write};

macro_rules! print_err {
    ($($arg:tt)*) => {{
            use std::io::prelude::*;
            write!(&mut ::std::io::stderr(), "{}\n", format_args!($($arg)*)).ok();
    }}
}

fn main() {
    let mut version: Vec<u8>;
    if let Ok(mut stat) = Command::new("git").args(&["describe", "--dirty"]).output() {
        if !stat.status.success() || stat.stdout.len() < 5 {
            print_err!("running revparse HEAD, since git describe --dirty failed");
            stat = Command::new("git")
                .args(&["revparse", "HEAD"])
                .output().expect("unable to call git revparse?");
        }

        print_err!("constructing version string");
        version = stat.stdout;
        if version.len() > 1 {
            let newlen = version.len()-1;
            version.truncate(newlen);
        }
        if version.len() < 5 {
            version.extend(b"(unknown version)");
        }
    } else {
        print_err!("Cannot run git executable?!");
        version = Vec::from("unknown");
    }

    // The following attempts to atomically create
    // $OUT_DIR/commit-info.txt, in case two cargos are running
    // simultaneously.  Rust does not document rename as atomic, but
    // on posix systems it currently is.
    let out_dir = std::path::PathBuf::from(std::env::var_os("OUT_DIR").unwrap());
    std::fs::File::create(out_dir.join("commit-info.txt~"))
        .unwrap()
        .write_all(&version)
        .unwrap();
    std::fs::rename(out_dir.join("commit-info.txt~"),
                    out_dir.join("commit-info.txt"))
        .expect("failed to rename commit-info.txt");
    println!("cargo:rerun-if-changed=.git/refs/heads/master");
    println!("cargo:rerun-if-changed=.git/HEAD");
}
