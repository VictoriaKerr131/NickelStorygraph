use std::fs::OpenOptions;
use std::io::Write;
use std::sync::{LazyLock, Mutex};

use anyhow::{Context, Result};
use chrono::Local;
use itertools::Itertools;
use serde_json::json;

use crate::config::CONFIG;
use crate::isbn::get_isbn;

#[allow(clippy::crate_in_macro_def)]
#[macro_export]
macro_rules! debug_log {
  ($($t:tt)*) => {{
    crate::utils::debug_log_inner(&format!($($t)*));
  }};
}

#[allow(clippy::crate_in_macro_def)]
#[macro_export]
macro_rules! log {
  ($($t:tt)*) => {{
    let msg = format!($($t)*);
    crate::utils::debug_log_inner(&msg);
    println!("{}", msg);
  }};
}

pub static VERSION: LazyLock<&str> = LazyLock::new(|| option_env!("VERSION").unwrap_or(env!("CARGO_PKG_VERSION")));

// In-memory buffer for the current CLI invocation; flushed to disk by write_logfile().
static LOG: LazyLock<Mutex<String>> = LazyLock::new(|| Mutex::new(String::new()));

pub fn debug_log_inner(msg: &str) {
  LOG
    .lock()
    .unwrap()
    .push_str(&format!("{} {msg}\n", Local::now().format("%c")));
}

/// Appends the current invocation's log buffer to a single persistent log file.
/// All CLI calls share one file so the full session history is in one place.
pub fn write_logfile() {
  let res = || -> Result<()> {
    let log_path = std::env::current_exe()
      .context("Failed to get current binary path")?
      .as_path()
      .parent()
      .context("Failed to get current binary directory")?
      .join("nickelstoregraph.log");

    let mut file = OpenOptions::new()
      .create(true)
      .append(true)
      .open(log_path)
      .context("Failed to open log file")?;

    file
      .write_all(LOG.lock().unwrap().as_bytes())
      .context("Failed to write log file")
  }();

  if let Err(e) = res {
    eprintln!(
      "Encountered an unexpected error. Please report this.<br><br>{:#}",
      e.chain().join("<br>> ")
    );
  }
}

/// Returns `(book_slug, isbns)`. If `linked_id` is set, returns it directly.
/// Otherwise extracts ISBNs from the Kobo content ID (EPUB path).
pub fn normalize_identifiers(linked_id: Option<String>, content_id: Option<&str>) -> (String, Vec<String>) {
  match (linked_id, content_id) {
    (Some(id), _) if !id.is_empty() => (id, Vec::new()),
    (_, Some(content_id)) => (String::new(), get_isbn(content_id)),
    _ => panic!("One of --content-id or --linked-id is required"),
  }
}

/// Emits a BOOK_NOT_FOUND JSON error to stdout and exits cleanly.
/// Used when a book can't be found by ISBN or linked ID so the hook
/// can show a user-facing message without treating it as a crash.
pub fn book_not_found(msg: &str) -> ! {
  log!(
    "BEGIN_JSON\n{}",
    json!({ "error_code": "BOOK_NOT_FOUND", "message": msg })
  );

  if CONFIG.debug {
    write_logfile();
  }

  std::process::exit(0);
}
