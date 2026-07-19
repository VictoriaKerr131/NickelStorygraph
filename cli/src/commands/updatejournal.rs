use std::collections::HashMap;
use std::fs;
use std::path::PathBuf;

use anyhow::{Context, Result};
use argh::FromArgs;
use rusqlite::{Connection, OpenFlags};
use serde_json::json;

use crate::{debug_log, log};
use crate::config::CONFIG;
use crate::storygraph;
use crate::utils::{VERSION, book_not_found, normalize_identifiers};

/// Sync Kobo highlights and notes for a book as StoryGraph journal entries.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update-journal")]
pub struct UpdateJournal {
  /// storygraph book slug
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID
  #[argh(option)]
  pub content_id: Option<String>,
}

pub fn run(args: UpdateJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  cleanup_old_sync_files();

  let content_id = args.content_id.clone().unwrap_or_default();
  let (book_id, isbns) = normalize_identifiers(args.linked_id, args.content_id.as_deref());

  let book_id = if !book_id.is_empty() {
    book_id
  } else if let Some(isbn) = isbns.first() {
    storygraph::find_by_isbn(isbn)?
      .map(|b| b.book_id)
      .unwrap_or_else(|| book_not_found("Couldn't find this book on StoryGraph. Please link book manually."))
  } else {
    book_not_found("No ISBN found. Please link book manually.")
  };

  let since = last_sync_timestamp(&content_id);
  debug_log!("update-journal: syncing annotations for {book_id} since {since}");

  let annotations = query_annotations(&content_id, &since)?;
  let count = annotations.len();

  if count == 0 {
    debug_log!("update-journal: no new annotations");
    log!("BEGIN_JSON\n{}", json!({ "success": true }));
    return Ok(());
  }

  let user_book = storygraph::get_user_book(&book_id)?;
  let progress_percent = user_book.progress_percent;
  debug_log!("update-journal: posting {count} annotations at {progress_percent}%");

  // One journal entry per highlight, so they read as individual entries on
  // StoryGraph instead of bunching into a single wall of text. If any entry
  // fails partway through, we bail without saving the sync timestamp so the
  // whole batch (including ones that already succeeded) is retried next
  // time rather than silently dropping the ones that failed.
  for (text, annotation) in &annotations {
    let note = if annotation.is_empty() {
      format!("<blockquote>{text}</blockquote>")
    } else {
      format!("<blockquote>{text}</blockquote><div><br>{annotation}</div>")
    };
    storygraph::insert_journal(&book_id, &note, progress_percent, "percentage")?;
  }

  let now = chrono::Utc::now().format("%Y-%m-%dT%H:%M:%SZ").to_string();
  if let Err(e) = save_sync_timestamp(&content_id, &now) {
    debug_log!("update-journal: failed to save sync timestamp: {e:#}");
  }

  log!("BEGIN_JSON\n{}", json!({ "success": true }));
  Ok(())
}

// One-time cleanup of the old per-book "annotations_<hash>.txt" files this
// used to scatter across the app directory. Their content can't be
// meaningfully migrated (the hash doesn't reveal which book it was for), so
// the first sync per book after upgrading re-checks full history — a
// one-time re-sync is preferable to leaving the clutter behind forever.
fn cleanup_old_sync_files() {
  let Ok(exe) = std::env::current_exe() else { return };
  let Some(dir) = exe.parent() else { return };
  let Ok(entries) = fs::read_dir(dir) else { return };

  for entry in entries.flatten() {
    let name = entry.file_name();
    let Some(name) = name.to_str() else { continue };
    if name.starts_with("annotations_") && name.ends_with(".txt") {
      let _ = fs::remove_file(entry.path());
    }
  }
}

// A single state file mapping content ID -> last annotation sync timestamp,
// instead of one file per book (which used to litter the app directory with
// opaquely-named "annotations_<hash>.txt" files, one per book ever synced).
fn sync_state_path() -> Result<PathBuf> {
  let exe = std::env::current_exe().context("Failed to get binary path")?;
  let dir = exe.parent().context("Failed to get binary directory")?;
  Ok(dir.join("annotation_sync_state.json"))
}

fn read_sync_state() -> HashMap<String, String> {
  sync_state_path()
    .ok()
    .and_then(|p| fs::read_to_string(p).ok())
    .and_then(|s| serde_json::from_str(&s).ok())
    .unwrap_or_default()
}

fn last_sync_timestamp(content_id: &str) -> String {
  read_sync_state()
    .get(content_id)
    .filter(|s| !s.is_empty())
    .cloned()
    .unwrap_or_else(|| "1970-01-01T00:00:00Z".to_owned())
}

fn save_sync_timestamp(content_id: &str, timestamp: &str) -> Result<()> {
  let mut state = read_sync_state();
  state.insert(content_id.to_owned(), timestamp.to_owned());
  fs::write(sync_state_path()?, serde_json::to_string(&state)?)
    .context("Failed to save annotation sync timestamp")
}

fn query_annotations(content_id: &str, since: &str) -> Result<Vec<(String, String)>> {
  let conn = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
    .context(format!("Failed to open database <i>{}</i>", &CONFIG.sqlite_path))?;

  conn
    .prepare(
      "SELECT COALESCE(Text, ''), COALESCE(Annotation, '')
       FROM Bookmark
       WHERE VolumeID = ?
         AND Hidden = 'false'
         AND (Text != '' OR Annotation != '')
         AND DateModified > ?
       ORDER BY DateModified ASC",
    )
    .context("Failed to prepare annotation query")?
    .query_map([content_id, since], |row| {
      Ok((row.get::<_, String>(0)?, row.get::<_, String>(1)?))
    })
    .context("Failed to query annotations")?
    .collect::<Result<Vec<_>, _>>()
    .context("Failed to collect annotations")
}
