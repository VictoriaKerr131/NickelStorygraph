use anyhow::Result;
use argh::FromArgs;

use crate::log;
use crate::storygraph;
use crate::utils::{VERSION, book_not_found, normalize_identifiers};

/// Get the current user's reading status for a book.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user-book")]
pub struct GetUserBook {
  /// storygraph book slug (stored in Kobo settings after linking)
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID (used to look up ISBN if not linked)
  #[argh(option)]
  pub content_id: Option<String>,
}

pub fn run(args: GetUserBook) -> Result<()> {
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

  let user_book = storygraph::get_user_book(&book_id)?;
  log!("BEGIN_JSON\n{}", serde_json::to_string(&user_book)?);
  Ok(())
}
