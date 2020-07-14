/// The version of fac
pub const VERSION : &str = git_version::git_version!(args = ["--always", "--dirty"], cargo_suffix="-cargo");
