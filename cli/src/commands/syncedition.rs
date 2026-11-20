use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::{VERSION, kobo_title, normalize_identifiers};

/// Sync the linked edition from the user's StoryGraph currently-reading shelf.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "sync-edition")]
pub struct SyncEdition {
  /// storygraph book id (linked edition)
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID
  #[argh(option)]
  pub content_id: Option<String>,
}

pub fn run(args: SyncEdition) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);
  let (book_id, _) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  anyhow::ensure!(!book_id.is_empty(), "No linked book found. Please link a book first.");
  let title = kobo_title(args.content_id.as_deref()).map(|(title, _)| title);
  // Only "not found" gets the shelf message; other errors (expired login,
  // Cloudflare block, network) are reported as they are.
  let found_id = storygraph::shelved_edition(&book_id, title.as_deref())?.unwrap_or_else(|| {
    panic!("This book isn't on your currently reading shelf on StoryGraph. Add it there to sync the edition.")
  });
  log!("BEGIN_JSON\n{}", json!({ "book_id": found_id }));
  Ok(())
}
