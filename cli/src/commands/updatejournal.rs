use std::collections::hash_map::DefaultHasher;
use std::fs;
use std::hash::{Hash, Hasher};
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

  // Batch all highlights into one journal entry so the user can review/delete
  // accidental highlights in Kobo before the next sync runs.
  let note: String = annotations
    .iter()
    .map(|(text, annotation)| {
      if annotation.is_empty() {
        format!("<blockquote>{text}</blockquote>")
      } else {
        format!("<blockquote>{text}</blockquote><div><br>{annotation}</div>")
      }
    })
    .collect();

  storygraph::insert_journal(&book_id, &note, progress_percent, "percentage")?;

  let now = chrono::Utc::now().format("%Y-%m-%dT%H:%M:%SZ").to_string();
  if let Err(e) = save_sync_timestamp(&content_id, &now) {
    debug_log!("update-journal: failed to save sync timestamp: {e:#}");
  }

  log!("BEGIN_JSON\n{}", json!({ "success": true }));
  Ok(())
}

fn sync_timestamp_path(content_id: &str) -> Result<PathBuf> {
  let mut hasher = DefaultHasher::new();
  content_id.hash(&mut hasher);
  let hash = hasher.finish();
  let exe = std::env::current_exe().context("Failed to get binary path")?;
  let dir = exe.parent().context("Failed to get binary directory")?;
  Ok(dir.join(format!("annotations_{hash:x}.txt")))
}

fn last_sync_timestamp(content_id: &str) -> String {
  sync_timestamp_path(content_id)
    .ok()
    .and_then(|p| fs::read_to_string(p).ok())
    .map(|s| s.trim().to_owned())
    .filter(|s| !s.is_empty())
    .unwrap_or_else(|| "1970-01-01T00:00:00Z".to_owned())
}

fn save_sync_timestamp(content_id: &str, timestamp: &str) -> Result<()> {
  fs::write(sync_timestamp_path(content_id)?, timestamp)
    .context("Failed to save annotation sync timestamp")
}

fn query_annotations(content_id: &str, since: &str) -> Result<Vec<(String, String)>> {
  let conn = Connection::open_with_flags(&CONFIG.sqlite_path, OpenFlags::SQLITE_OPEN_READ_ONLY)
    .context(format!("Failed to open database <i>{}</i>", &CONFIG.sqlite_path))?;

  conn
    .prepare(
      "SELECT Text, COALESCE(Annotation, '')
       FROM Bookmark
       WHERE VolumeID = ?
         AND Hidden = 'false'
         AND Text != ''
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
