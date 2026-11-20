use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph::{self, Book, title_match_score};
use crate::utils::{VERSION, book_not_found, kobo_title, normalize_identifiers};

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

  /// the user agreed to start a reread if the book is finished on StoryGraph
  #[argh(switch)]
  pub reread: bool,
}

pub fn run(args: Update) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let content_id = args.content_id.clone();
  let (book_id, isbns) = normalize_identifiers(args.linked_id, content_id.as_deref());
  let meta = kobo_title(content_id.as_deref());
  let title = meta.as_ref().map(|(title, _)| title.as_str());

  let book_id = if !book_id.is_empty() {
    book_id
  } else {
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

  // Push to whichever edition is actually on the shelf (e.g. an audiobook edition the
  // user switched to on StoryGraph), so we don't add the Kobo edition as a second entry.
  // Failing that, to another edition of the work the user has shelved (e.g. to-read).
  let shelf_id = match storygraph::shelved_edition(&book_id, title)? {
    Some(id) => id,
    None => match &meta {
      Some((title, author)) => storygraph::searched_edition(&book_id, title, author)?.unwrap_or_else(|| book_id.clone()),
      None => book_id.clone(),
    },
  };
  let user_book = match storygraph::update_progress(&shelf_id, "percentage", args.value, title, args.reread) {
    Ok(user_book) => user_book,
    // Finished and no reread requested: tell the hook, which asks the user (or, for an
    // auto-sync, just remembers the status for the prompt shown when the book is opened).
    Err(e) if e.is::<storygraph::BookFinished>() => {
      log!("BEGIN_JSON\n{}", json!({
        "error_code": "BOOK_FINISHED",
        "resolved_book_id": book_id,
        "status_id": 3,
      }));
      return Ok(());
    }
    Err(e) => return Err(e),
  };
  let mut resp = serde_json::to_value(&user_book)?;
  resp["resolved_book_id"] = json!(book_id);
  log!("BEGIN_JSON\n{}", resp);
  Ok(())
}

/// Picks the best match from `results` for `title`, requiring a near-exact match (≥ 90%
/// of meaningful query words present). This runs with no confirmation step, so it must be
/// strict enough to reject other volumes/editions of the same series (which can otherwise
/// share every word but a volume number) rather than silently linking the wrong book.
/// Returns None if nothing scores well enough.
fn best_title_match(results: Vec<Book>, title: &str) -> Option<Book> {
  results
    .into_iter()
    .filter_map(|book| {
      let score = title_match_score(title, &book.title);
      if score >= 0.9 { Some((book, score)) } else { None }
    })
    .max_by(|(_, a), (_, b)| a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal))
    .map(|(book, _)| book)
}
