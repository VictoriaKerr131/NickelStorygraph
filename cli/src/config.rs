use std::fs;
use std::sync::LazyLock;

use anyhow::{Context, Result};
use itertools::Itertools;
use serde::{Deserialize, Deserializer, Serialize, de};

#[derive(Serialize, PartialEq, Debug)]
#[serde(rename_all = "lowercase")]
pub enum SyncBookmarks {
  Always,
  Never,
  Finished,
}

impl<'de> Deserialize<'de> for SyncBookmarks {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    match String::deserialize(deserializer)? {
      s if s.eq_ignore_ascii_case("Always") => Ok(SyncBookmarks::Always),
      s if s.eq_ignore_ascii_case("Never") => Ok(SyncBookmarks::Never),
      s if s.eq_ignore_ascii_case("Finished") => Ok(SyncBookmarks::Finished),
      s => Err(de::Error::custom(format!(
        "<i>{s}</i> is not a valid <i>sync_bookmarks</i> value"
      ))),
    }
  }
}

#[derive(Serialize, PartialEq, Debug)]
#[serde(rename_all = "lowercase")]
pub enum SyncOnClose {
  Always,
  Never,
  Number(u8),
}

impl<'de> Deserialize<'de> for SyncOnClose {
  fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
  where
    D: Deserializer<'de>,
  {
    let s = String::deserialize(deserializer)?;

    if s.eq_ignore_ascii_case("Always") {
      Ok(SyncOnClose::Always)
    } else if s.eq_ignore_ascii_case("Never") {
      Ok(SyncOnClose::Never)
    } else if let Ok(n) = s.parse::<u8>()
      && n > 1
      && n <= 100
    {
      Ok(SyncOnClose::Number(n))
    } else {
      Err(de::Error::custom(format!(
        "<i>{s}</i> is not a valid <i>sync_on_close</i> value"
      )))
    }
  }
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(default)]
pub struct Config {
  pub session_cookie: String,
  pub remember_user_token: String,
  pub user_slug: String,
  pub auto_sync_default: bool,
  pub debug: bool,
  pub sqlite_path: String,
  pub sync_bookmarks: SyncBookmarks,
  pub sync_daily: u8,
  pub sync_on_close: SyncOnClose,
  pub threshold: u8,
}

impl Default for Config {
  fn default() -> Self {
    Self {
      session_cookie: String::new(),
      remember_user_token: String::new(),
      user_slug: String::new(),
      auto_sync_default: false,
      debug: false,
      sqlite_path: "/mnt/onboard/.kobo/KoboReader.sqlite".into(),
      sync_bookmarks: SyncBookmarks::Never,
      sync_daily: 0,
      sync_on_close: SyncOnClose::Never,
      threshold: 0,
    }
  }
}

pub static CONFIG: LazyLock<Config> = LazyLock::new(|| {
  let config = || -> Result<Config> {
    let current_exe = std::env::current_exe().context("Failed to get current binary path")?;
    let exe_dir = current_exe.parent().context("Failed to get current binary directory")?;
    let config_path = exe_dir.join("config.ini");

    // Create config.ini if it doesn't exist yet.
    if !config_path.exists() {
      // Copy config_example.ini so the user starts with a commented, documented file.
      // Fall back to a generated default if the example isn't present.
      let example_path = exe_dir.join("config_example.ini");
      if example_path.exists() {
        fs::copy(&example_path, &config_path).context("Failed to copy config_example.ini to config.ini")?;
      } else {
        let ini = serini::to_string(&Config::default()).context("Failed to serialize default config")?;
        fs::write(&config_path, ini).context("Failed to write default config")?;
      }
    }

    let content = fs::read_to_string(&config_path)
      .context("Failed to read config file")?
      .replace("[General]", "");
    let config: Config = serini::from_str(&content).context("Failed to parse config file")?;

    Ok(Config {
      sqlite_path: exe_dir
        .join(config.sqlite_path)
        .to_str()
        .context("Failed to get SQLite path")?
        .to_string(),
      ..config
    })
  }();

  match config {
    Ok(config) => config,
    Err(e) => {
      panic!(
        "Encountered an unexpected error. Please report this.<br><br>{:#}",
        e.chain().join("<br>> ")
      );
    }
  }
});
