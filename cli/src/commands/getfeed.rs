use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::VERSION;

/// Fetch the community activity feed from StoryGraph.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-feed")]
pub struct GetFeed {}

pub fn run(_args: GetFeed) -> Result<()> {
  log!("{} get-feed", &*VERSION);
  let items = storygraph::get_feed()?;
  log!("BEGIN_JSON\n{}", json!({ "items": items }));
  Ok(())
}
