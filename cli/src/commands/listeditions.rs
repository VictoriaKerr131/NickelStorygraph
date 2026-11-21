use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::{UserError, VERSION};

const EDITIONS_BLOCKED: &str = "StoryGraph is currently blocking apps from loading book pages, so editions can't be viewed or switched on your Kobo at this time. This may be temporary.<br><br>Please try again later, or change editions on the StoryGraph website and then use the <i>Sync edition from web</i> option in the in-book menu.";

/// List available editions of a book.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-editions")]
pub struct ListEditions {
  /// storygraph book slug
  #[argh(option)]
  pub book_id: String,

  /// reading format filter: 0=all, 1=print, 4=ebook
  #[argh(option, default = "0")]
  pub reading_format: i32,

  /// language filter
  #[argh(option, default = "String::new()")]
  pub language: String,
}

pub fn run(args: ListEditions) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);
  let editions = storygraph::list_editions(&args.book_id, args.reading_format, &args.language).map_err(|e| {
    if storygraph::is_book_page_blocked(&e) { UserError(EDITIONS_BLOCKED.to_owned()).into() } else { e }
  })?;
  log!("BEGIN_JSON\n{}", json!({ "editions": editions }));
  Ok(())
}
