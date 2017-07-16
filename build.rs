
use std::process::Command;
use std::io::{Write};

macro_rules! print_err {
    ($($arg:tt)*) => (
        {
            use std::io::prelude::*;
            write!(&mut ::std::io::stderr(), "{}\n", format_args!($($arg)*)).ok();
        }
    )
}

fn main() {
    print_err!("removing old src/version.rs");
    std::fs::remove_file("src/version.rs").ok();
    print_err!("running git describe --dirty");
    let mut stat = Command::new("git")
        .args(&["describe", "--dirty"])
        .output().unwrap();
    if !stat.status.success() || stat.stdout.len() < 5 {
        print_err!("running revparse HEAD, since git describe --dirty failed");
        stat = Command::new("git")
            .args(&["revparse", "HEAD"])
            .output().unwrap();
    }

    print_err!("constructing version string");
    let mut version = stat.stdout;
    if version.len() > 1 {
        let newlen = version.len()-1;
        version.truncate(newlen);
    }
    if version.len() < 5 {
        version.extend(b"(unknown version)");
    }

    print_err!("creating src/version.rs");
    let mut file = std::fs::File::create("src/version.rs").expect("error creating version.rs");
    print_err!("writing to src/version.rs");
    file.write(b"/// The version of fac\npub static VERSION: &'static str = \"")
        .expect("error writing to file");
    file.write(&version).expect("error writing to file");
    file.write(b"\";\n").expect("error writing to file");
    print_err!("all done writing to src/version.rs");
}
