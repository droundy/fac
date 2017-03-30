extern crate libc;

use libc::c_char;
use libc::c_int;

use std::ffi::OsString;
use std::os::unix::ffi::OsStringExt;

use std::ffi::CString;

mod git;

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

    git::go_to_top();
    unsafe {
        initialize_starting_time();
        run_fac(argc, c_argv.as_ptr());
    };
}
