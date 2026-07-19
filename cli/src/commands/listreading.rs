use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::VERSION;

/// List books on the currently reading shelf with progress info.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "list-reading")]
pub struct ListReading {}

pub fn run(_args: ListReading) -> Result<()> {
  log!("{} list-reading", &*VERSION);
  let books = storygraph::list_reading()?;
  log!("BEGIN_JSON\n{}", json!({ "books": books }));
  Ok(())
}
