use anyhow::Result;
use argh::FromArgs;
use rusqlite::{Connection, OpenFlags};
use serde_json::json;

use crate::config::CONFIG;
use crate::log;
use crate::storygraph::{self, Book, title_match_score};
use crate::utils::{VERSION, book_not_found, normalize_identifiers};

/// Update reading progress for a book.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "update")]
pub struct Update {
  /// storygraph book slug
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID
  #[argh(option)]
  pub content_id: Option<String>,

  /// progress percentage (0–100)
  #[argh(option)]
  pub value: u32,
}

pub fn run(args: Update) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let content_id = args.content_id.clone();
  let (book_id, isbns) = normalize_identifiers(args.linked_id, content_id.as_deref());

  let book_id = if !book_id.is_empty() {
    book_id
  } else {
    let meta = book_title_query(content_id.as_deref());

    // Before doing an ISBN/search lookup, check if the book is already on the user's
    // currently-reading shelf. If so, link to that edition and let the hook ask the
    // user whether to keep StoryGraph's progress or push Kobo's.
    if let Some((ref title, _)) = meta {
      if let Ok(Some((shelf_id, shelf_progress))) = storygraph::find_on_shelf(title) {
        log!("BEGIN_JSON\n{}", serde_json::json!({
          "resolved_book_id": shelf_id,
          "needs_shelf_confirmation": true,
          "progress_percent": shelf_progress,
        }));
        return Ok(());
      }
    }

    let found = isbns
      .iter()
      .find_map(|isbn| {
        let result = storygraph::find_by_isbn(isbn).ok().flatten()?;
        // Reject ISBN results that don't match the Kobo title — the EPUB's ISBN metadata
        // sometimes points to a different book (e.g. a sampler or wrong edition).
        if let Some((ref title, _)) = meta {
          if title_match_score(title, &result.title) < 0.3 {
            return None;
          }
        }
        Some(result)
      })
      .or_else(|| {
        let (title, author) = meta.as_ref()?;
        let results = storygraph::search(&format!("{title} {author}")).ok()?;
        best_title_match(results, title)
      });

    let initial_id = match found {
      Some(book) => book.book_id,
      None => book_not_found("Couldn't find this book on StoryGraph. Please link it manually."),
    };
    storygraph::prefer_digital_edition(&initial_id)?
  };

  let user_book = storygraph::update_progress(&book_id, "percentage", args.value)?;
  let mut resp = serde_json::to_value(&user_book)?;
  resp["resolved_book_id"] = json!(book_id);
  log!("BEGIN_JSON\n{}", resp);
  Ok(())
}

/// Picks the best match from `results` for `title`, requiring at least 50% of meaningful
/// query words to appear in the result title. Returns None if nothing scores well enough.
fn best_title_match(results: Vec<Book>, title: &str) -> Option<Book> {
  results
    .into_iter()
    .filter_map(|book| {
      let score = title_match_score(title, &book.title);
      if score >= 0.5 { Some((book, score)) } else { None }
    })
    .max_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal))
    .map(|(book, _)| book)
}

fn book_title_query(content_id: Option<&str>) -> Option<(String, String)> {
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
