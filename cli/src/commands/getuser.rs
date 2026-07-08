use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::{debug_log, log};
use crate::storygraph;
use crate::utils::VERSION;

/// Verify session and return basic user info.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "get-user")]
pub struct GetUser {}

pub fn run(args: GetUser) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);
  match storygraph::get_user() {
    Ok(user) => log!("BEGIN_JSON\n{}", serde_json::to_string(&user)?),
    Err(e) => {
      // Non-fatal: settings dialog shows "Unknown" instead of crashing.
      // Most likely cause is user_slug not set in config.ini.
      debug_log!("get_user failed (set user_slug in config.ini to avoid this): {e:#}");
      log!("BEGIN_JSON\n{}", json!({ "username": "", "slug": "" }));
    }
  }
  Ok(())
}
