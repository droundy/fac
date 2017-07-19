extern crate fac;

use std::io::Write;

#[test]
fn it_works() {
    assert!(fac::version::VERSION.len() > 0);
    create_repository(std::collections::HashMap::new()).unwrap();
}

fn create_repository(files: std::collections::HashMap<std::path::PathBuf, Vec<u8>>) -> std::io::Result<()> {
    println!("remove test-repository");
    std::fs::remove_dir_all("test-repository").ok();
    println!("create test-repository");
    std::fs::create_dir_all("test-repository")?;
    std::env::set_current_dir("test-repository")?;
    git_init()?;
    for (fname, contents) in files {
        let mut f = std::fs::File::create(&fname)?;
        f.write(&contents)?;
        fac::git::add(&fname)?;
    }
    Ok(())
}

/// git add a file or more
pub fn git_init() -> std::io::Result<()> {
    let s = std::process::Command::new("git").arg("init").output()?;
    if s.status.success() {
        Ok(())
    } else {
        Err(std::io::Error::new(std::io::ErrorKind::Other,
                                String::from_utf8_lossy(&s.stderr).into_owned()))
    }
}
