use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::{VERSION, book_not_found, normalize_identifiers};

/// Create a journal entry (note or highlight) on StoryGraph.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "insert-journal")]
pub struct InsertJournal {
  /// storygraph book slug
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID
  #[argh(option)]
  pub content_id: Option<String>,

  /// journal entry body text
  #[argh(option)]
  pub text: String,

  /// reading progress percentage at time of entry (0–100)
  #[argh(option, default = "0")]
  pub percentage: u32,

  /// privacy level — ignored (StoryGraph does not expose per-entry privacy via the web API)
  #[argh(option, default = "String::new()")]
  pub privacy: String,
}

pub fn run(args: InsertJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

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

  storygraph::insert_journal(&book_id, &args.text, args.percentage, "percentage")?;
  log!("BEGIN_JSON\n{}", json!({ "success": true }));
  Ok(())
}
