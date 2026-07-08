use anyhow::Result;
use argh::FromArgs;
use serde_json::json;

use crate::log;
use crate::storygraph;
use crate::utils::{VERSION, book_not_found, normalize_identifiers};

/// Change the reading status of a book, or submit a review.
#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand, name = "set-user-book")]
pub struct SetUserBook {
  /// storygraph book slug
  #[argh(option)]
  pub linked_id: Option<String>,

  /// kobo content ID
  #[argh(option)]
  pub content_id: Option<String>,

  /// numeric status: 1=to-read, 2=reading, 3=read, 4=paused, 5=dnf
  #[argh(option)]
  pub status: Option<u8>,

  /// star rating (0.0–5.0, quarter-star increments)
  #[argh(option)]
  pub rating: Option<f32>,

  /// review text
  #[argh(option)]
  pub text: Option<String>,

  /// mood ID 1–14 (repeat for multiple: --mood 1 --mood 4)
  #[argh(option)]
  pub mood: Vec<u8>,

  /// pace: slow, medium, or fast
  #[argh(option)]
  pub pace: Option<String>,

  /// plot or character driven: Plot, Character, A mix, N/A
  #[argh(option)]
  pub character_or_plot_driven: Option<String>,

  /// strong character development: Yes, No, It's complicated, N/A
  #[argh(option)]
  pub strong_character_development: Option<String>,

  /// loveable characters: Yes, No, It's complicated, N/A
  #[argh(option)]
  pub loveable_characters: Option<String>,

  /// diverse characters: Yes, No, It's complicated, N/A
  #[argh(option)]
  pub diverse_characters: Option<String>,

  /// character flaws main focus: Yes, No, It's complicated, N/A
  #[argh(option)]
  pub flawed_characters: Option<String>,
}

pub fn run(args: SetUserBook) -> Result<()> {
  log!("{} {:?}", &*VERSION, args);

  let (book_id, isbns) = normalize_identifiers(args.linked_id, args.content_id.as_deref());
  let book_id = if !book_id.is_empty() {
    book_id
  } else if let Some(isbn) = isbns.first() {
    storygraph::find_by_isbn(isbn)?
      .map(|b| b.book_id)
      .unwrap_or_else(|| book_not_found("Couldn't find this book on StoryGraph. Please link book manually."))
  } else {
    book_not_found("No ISBN found. Please link book manually.")
  };

  if let Some(status_id) = args.status {
    let user_book = storygraph::update_status(&book_id, status_id)?;
    log!("BEGIN_JSON\n{}", serde_json::to_string(&user_book)?);
  } else {
    storygraph::submit_review(
      &book_id,
      args.rating,
      &args.text.unwrap_or_default(),
      &args.mood,
      &args.pace.unwrap_or_default(),
      &args.character_or_plot_driven.unwrap_or_default(),
      &args.strong_character_development.unwrap_or_default(),
      &args.loveable_characters.unwrap_or_default(),
      &args.diverse_characters.unwrap_or_default(),
      &args.flawed_characters.unwrap_or_default(),
    )?;
    log!("BEGIN_JSON\n{}", json!({ "success": true }));
  }

  Ok(())
}
