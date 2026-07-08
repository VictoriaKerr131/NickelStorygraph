use anyhow::Result;
use argh::FromArgs;
use rusqlite::{Connection, OpenFlags};
use serde_json::json;

use crate::config::CONFIG;
use crate::log;
use crate::storygraph::{self, Book};
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

/// Returns what fraction of the meaningful words in `query_title` appear in `result_title`.
/// Stop words and single-character tokens are excluded from scoring.
fn title_match_score(query_title: &str, result_title: &str) -> f32 {
  const STOPS: &[&str] = &["the", "a", "an", "of", "in", "by", "to", "and", "or", "for", "at", "its", "is"];

  let tokenize = |s: &str| -> Vec<String> {
    s.to_lowercase()
      .split(|c: char| !c.is_alphabetic())
      .filter(|w| w.len() > 1 && !STOPS.contains(w))
      .map(str::to_owned)
      .collect()
  };

  let query_words = tokenize(query_title);
  if query_words.is_empty() {
    return 0.0;
  }

  let result_words: std::collections::HashSet<String> = tokenize(result_title).into_iter().collect();
  let matches = query_words.iter().filter(|w| result_words.contains(*w)).count();
  matches as f32 / query_words.len() as f32
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
