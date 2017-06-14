//! Environment hashing

use std;
use std::hash::{Hasher, Hash};
use std::collections::hash_map::DefaultHasher;

/// VERBOSITY is used to enable our vprintln macro to know the
/// verbosity.  This is a bit ugly, but is needed due to rust macros
/// being hygienic.
static mut ENVHASH: u64 = 0;

/// Hash the environment
pub fn hash() -> u64 {
    if unsafe { ENVHASH } == 0 {
        let mut env: Vec<_> = std::env::vars_os().collect();
        env.sort();
        let mut hasher = DefaultHasher::new();
        for &(ref k,ref v) in &env {
            if !IGNORE_THESE.contains(&k.to_string_lossy().as_ref()) {
                k.hash(&mut hasher);
                v.hash(&mut hasher);
            }
        }
        unsafe { ENVHASH = hasher.finish(); }
    };
    unsafe { ENVHASH }
}

const IGNORE_THESE: &'static [&'static str] = &[
  "COLORTERM",
  "DBUS_SESSION_BUS_ADDRESS",
  "DESKTOP_SESSION",
  "DESKTOP_STARTUP_ID",
  "DISPLAY",
  "GCONF_GLOBAL_LOCKS",
  "GDM_LANG",
  "GDMSESSION",
  "GIO_LAUNCHED_DESKTOP_FILE_PID",
  "GIO_LAUNCHED_DESKTOP_FILE",
  "GJS_DEBUG_OUTPUT",
  "GJS_DEBUG_TOPICS",
  "GNOME_DESKTOP_SESSION_ID",
  "GNOME_KEYRING_CONTROL",
  "GNOME_KEYRING_PID",
  "GPG_AGENT_INFO",
  "OLDPWD",
  "MAIL",
  "NCURSES_",
  "PS1",
  "PWD",
  "SESSION_MANAGER",
  "SHLVL",
  "SSH_AGENT_PID",
  "SSH_AUTH_SOCK",
  "SSH_CLIENT",
  "SSH_CONNECTION",
  "SSH_TTY",
  "TERM",
  "USERNAME", // not sure why, but this is sporadically defined
  "VTE_VERSION",
  "WINDOWID",
  "WINDOWPATH",
  "XAUTHORITY",
  "XDG_CURRENT_DESKTOP",
  "XDG_DATA_DIRS",
  "XDG_MENU_PREFIX",
  "XDG_SEAT",
  "XDG_SESSION_COOKIE",
  "XDG_SESSION_DESKTOP",
  "XDG_SESSION_ID",
  "XDG_VTNR",
  "_",
];
