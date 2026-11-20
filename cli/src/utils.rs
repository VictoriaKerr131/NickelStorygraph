use std::error::Error;
use std::fmt;
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
      .join("nickelstorygraph.log");

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

/// Removes emoji, which the Kobo's fonts can't show (they render as empty squares),
/// and tidies the spaces they leave behind. Line breaks are kept.
pub fn strip_emoji(s: &str) -> String {
  let is_emoji = |c: char| {
    matches!(c as u32,
      0x1F000..=0x1FAFF    // emoji, pictographs, flags, skin tones
      | 0x2600..=0x27BF    // misc symbols and dingbats (☀ ✨ ❤ ✅)
      | 0x2B00..=0x2BFF    // misc symbols and arrows (⭐ ⬆)
      | 0x2300..=0x23FF    // misc technical (⌛ ⏰ ⏳)
      | 0xFE00..=0xFE0F    // variation selectors
      | 0x200D             // zero-width joiner
      | 0x20E3             // combining keycap
      | 0xE0000..=0xE007F  // tags (subdivision flags)
    )
  };

  let mut out = String::with_capacity(s.len());
  for c in s.chars().filter(|&c| !is_emoji(c)) {
    if c == ' ' && (out.is_empty() || out.ends_with(' ') || out.ends_with('\n')) {
      continue;
    }
    if c == '\n' && out.ends_with(' ') {
      out.pop();
    }
    out.push(c);
  }
  out.trim().to_owned()
}

/// The book's (title, author) from the Kobo database.
pub fn kobo_title(content_id: Option<&str>) -> Option<(String, String)> {
  use rusqlite::{Connection, OpenFlags};

  let content_id = content_id?;
  let conn = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY).ok()?;
  conn
    .query_row(
      "SELECT Title, Attribution FROM content WHERE ContentID = ? LIMIT 1",
      [content_id],
      |row| Ok((row.get::<_, String>(0)?, row.get::<_, String>(1).unwrap_or_default())),
    )
    .ok()
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

/// An expected, user-facing error (expired login, Cloudflare check, …). main.rs
/// shows its message as is instead of "Encountered an unexpected error".
#[derive(Debug)]
pub struct UserError(pub String);

impl fmt::Display for UserError {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    f.write_str(&self.0)
  }
}

impl Error for UserError {}
