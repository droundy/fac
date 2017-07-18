
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

    // The following attempts to atomically create version.rs, in case
    // two cargos are running simultaneously.  Rust does not document
    // rename as atomic, but on posix systems it currently is.
    {
        print_err!("creating src/version.rs~, and I am in {:?}", std::env::current_dir());
        let mut file = std::fs::File::create("src/version.rs~").expect("error creating version.rs");
        print_err!("writing to src/version.rs");
        file.write(b"/// The version of fac\npub static VERSION: &'static str = \"")
            .expect("error writing to file");
        file.write(&version).expect("error writing to file");
        file.write(b"\";\n").expect("error writing to file");
        print_err!("all done writing to src/version.rs~");
    }
    print_err!("mv src/version.rs~ src/version.rs");
    std::fs::rename("src/version.rs~", "src/version.rs")
        .expect("failed to rename version.rs");
}
