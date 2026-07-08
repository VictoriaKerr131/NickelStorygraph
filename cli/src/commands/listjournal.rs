use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::{VERSION, normalize_identifiers};

/// List reading journal entries for a book from StoryGraph.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-journal")]
pub struct ListJournal {
  /// storygraph book slug
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID
  #[argh(option)]
  pub content_id: Option<String>,

  /// maximum number of entries to return
  #[argh(option, default = "10")]
  pub limit: usize,

  /// number of entries to skip
  #[argh(option, default = "0")]
  pub offset: usize,
}

pub fn run(args: ListJournal) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (book_id, _) = normalize_identifiers(args.linked_id, args.content_id.as_deref());

  if book_id.is_empty() {
    log!("BEGIN_JSON\n{}", json!({ "reading_journals": [] }));
    return Ok(());
  }

  let entries = storygraph::list_journal(&book_id, args.limit, args.offset)?;
  log!("BEGIN_JSON\n{}", json!({ "reading_journals": entries }));
  Ok(())
}
