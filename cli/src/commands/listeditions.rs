use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::VERSION;

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
  let editions = storygraph::list_editions(&args.book_id, args.reading_format, &args.language)?;
  log!("BEGIN_JSON\n{}", json!({ "editions": editions }));
  Ok(())
}
