extern crate libc;

use libc::c_char;
use libc::c_int;

use std::ffi::OsString;
use std::os::unix::ffi::OsStringExt;

use std::ffi::CString;

#[link(name="fac")]
extern "C" {
    fn run_fac(argc: c_int, argv: *const *const c_char);
    fn initialize_starting_time();
}

fn main() {
    // create a vector of zero terminated strings
    let args = std::env::args()
        .map(|arg| CString::new(arg).unwrap() )
        .collect::<Vec<CString>>();
    // convert the strings to raw pointers
    let mut c_argv = args.iter().map(|arg| arg.as_ptr())
        .collect::<Vec<*const c_char>>();
    c_argv.push(std::ptr::null());
    let argc = c_argv.len() as c_int -1;

    go_to_git_top();
    unsafe {
        initialize_starting_time();
        run_fac(argc, c_argv.as_ptr());
    };
}

pub fn go_to_git_top() -> std::path::PathBuf {
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
