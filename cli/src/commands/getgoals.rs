use anyhow::Result;
use argh::FromArgs;

use crate::log;
use crate::storygraph;
use crate::utils::VERSION;

/// Fetch reading goals from the StoryGraph challenges dashboard.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-goals")]
pub struct GetGoals {}

pub fn run(_args: GetGoals) -> Result<()> {
  log!("{} get-goals", &*VERSION);
  let goals = storygraph::get_goals()?;
  log!("BEGIN_JSON\n{}", serde_json::to_string(&goals).unwrap());
  Ok(())
}
