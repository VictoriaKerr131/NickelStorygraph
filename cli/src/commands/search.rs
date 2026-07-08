use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::VERSION;

/// Search for books on StoryGraph.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "search")]
pub struct Search {
  /// search query (title, author, or ISBN)
  #[argh(option)]
  pub query: String,

  /// max results to return (default 20)
  #[argh(option, default = "20")]
  pub limit: usize,

  /// page offset (1-based, unused — kept for hook compatibility)
  #[argh(option, default = "1")]
  pub page: usize,
}

pub fn run(args: Search) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let mut books = storygraph::search(&args.query)?;
  books.truncate(args.limit);

  let total = books.len();
  log!("BEGIN_JSON\n{}", json!({ "results": books, "total": total }));
  Ok(())
}
