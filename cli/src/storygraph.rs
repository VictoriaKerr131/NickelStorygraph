// HTTP client for the StoryGraph unofficial web API.
//
// StoryGraph has no public API. All interactions mirror what a browser does:
// session-cookie auth, HTML-scraped responses for reads, and HTML form POSTs
// for writes. A CSRF token is extracted from the page HTML before each POST.
//
// Auth cookies come from browser DevTools and live in config.ini:
//   session_cookie     → _storygraph_session
//   remember_user_token → remember_user_token

use std::sync::LazyLock;

use anyhow::{Context, Result, bail};
use reqwest::blocking::{Client, Response};
use reqwest::cookie::Jar;
use reqwest::{Certificate, StatusCode, Url};
use retry::delay::{Exponential, jitter};
use retry::retry;
use scraper::{Html, Selector};
use serde::Serialize;
use serde_json::{json, Value};
use std::sync::Arc;

use crate::config::CONFIG;
use crate::debug_log;

pub const BASE_URL: &str = "https://app.thestorygraph.com";

const USER_AGENT: &str =
  "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36";

// ── Output types (serialized to JSON for the hook) ───────────────────────────

fn serialize_author_as_array<S: serde::Serializer>(val: &String, s: S) -> Result<S::Ok, S::Error> {
  use serde::ser::SerializeSeq;
  let mut seq = s.serialize_seq(Some(1))?;
  seq.serialize_element(val.as_str())?;
  seq.end()
}

#[derive(Serialize, Debug)]
pub struct Book {
  #[serde(rename = "id")]
  pub book_id: String,
  pub title: String,
  #[serde(rename = "authors", serialize_with = "serialize_author_as_array")]
  pub author: String,
  #[serde(rename = "image")]
  pub cover_url: Option<String>,
  pub pages: u32,
}

#[derive(Serialize, Debug)]
pub struct JournalActivity {
  pub event: String,
  pub action_at: String,
  #[serde(skip_serializing_if = "String::is_empty")]
  pub entry: String,
  pub metadata: Value,
}

#[derive(Serialize, Debug)]
pub struct UserBook {
  pub book_id: String,
  pub status_id: Option<u8>,
  pub progress_percent: u32,
  pub progress_pages: u32,
  pub book_pages: u32,
  pub progress_type: String,
  pub edition_format: String,
}

#[derive(Serialize, Debug)]
pub struct Edition {
  pub book_id: String,
  pub title: String,
  #[serde(rename = "edition_format")]
  pub format: String,
  pub pages: Option<u32>,
  pub isbn: String,
  pub language: String,
  pub publisher: String,
  #[serde(rename = "release_date")]
  pub pub_date: String,
  #[serde(rename = "image")]
  pub cover_url: Option<String>,
}

#[derive(Serialize, Debug)]
pub struct User {
  pub username: String,
  pub slug: String,
}

// Page-level state extracted from /books/:slug before any mutating POST.
struct PageData {
  csrf: String,
  book_pages: String,
  last_pages: String,
  last_percent: String,
}

// ── Singleton client ──────────────────────────────────────────────────────────

static CLIENT: LazyLock<Result<Client, reqwest::Error>> = LazyLock::new(|| {
  let base: Url = BASE_URL.parse().expect("BASE_URL is valid");
  let jar = Arc::new(Jar::default());

  // Pre-seed auth cookies from config.
  jar.add_cookie_str(
    &format!("_storygraph_session={}", CONFIG.session_cookie),
    &base,
  );
  jar.add_cookie_str(
    &format!("remember_user_token={}", CONFIG.remember_user_token),
    &base,
  );
  jar.add_cookie_str("cookies_popup_seen=yes", &base);
  jar.add_cookie_str("plus_popup_seen=yes", &base);

  Client::builder()
    .cookie_provider(jar)
    .user_agent(USER_AGENT)
    .tls_certs_only(
      webpki_root_certs::TLS_SERVER_ROOT_CERTS
        .iter()
        .map(|cert| Certificate::from_der(cert))
        .collect::<Result<Vec<_>, _>>()?,
    )
    .build()
});

fn client() -> Result<&'static Client> {
  CLIENT.as_ref().context("Failed to construct HTTP client")
}

// ── Guards ────────────────────────────────────────────────────────────────────

fn assert_credentials() {
  assert!(
    !CONFIG.session_cookie.is_empty(),
    "Please set <i>session_cookie</i> in <i>.adds/NickelStorygraph/config.ini</i>. Obtain it from browser DevTools → Application → Cookies → app.thestorygraph.com."
  );
}

/// Returns Err if the final response URL ended up on the sign-in page.
fn check_session(response: &Response) -> Result<()> {
  if response.url().path().contains("/users/sign_in") {
    bail!(
      "StoryGraph session has expired. Please update <i>session_cookie</i> and <i>remember_user_token</i> in <i>.adds/NickelStorygraph/config.ini</i>."
    );
  }
  Ok(())
}

// ── Low-level HTTP ────────────────────────────────────────────────────────────

fn get(path: &str) -> Result<String> {
  let url = format!("{BASE_URL}{path}");
  debug_log!("GET {url}");

  let res = retry(Exponential::from_millis(500).map(jitter).take(3), || {
    client()?.get(&url).send().context(format!("GET {url} failed"))
  })
  .map_err(|e| e.error)?;

  check_session(&res)?;
  res.error_for_status_ref().context(format!("GET {url} returned error status"))?;
  res.text().context(format!("Failed to read body from GET {url}"))
}

/// Uses regular browser headers instead of XHR.
/// Needed for endpoints (like /reviews) that behave differently for AJAX.
fn post_form_browser(path: &str, fields: &[(String, String)]) -> Result<String> {
  let url = format!("{BASE_URL}{path}");
  debug_log!("POST {url} (browser)");

  let res = retry(Exponential::from_millis(500).map(jitter).take(3), || {
    client()?
      .post(&url)
      .header("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8")
      .header("Origin", BASE_URL)
      .header("Referer", &url)
      .form(fields)
      .send()
      .context(format!("POST {url} failed"))
  })
  .map_err(|e| e.error)?;

  if res.status() == StatusCode::FOUND {
    let location = res.headers().get("location").and_then(|v| v.to_str().ok()).unwrap_or("");
    if location.contains("/users/sign_in") {
      bail!(
        "StoryGraph session has expired. Please update <i>session_cookie</i> and <i>remember_user_token</i> in <i>.adds/NickelStorygraph/config.ini</i>."
      );
    }
    return Ok(String::new());
  }

  check_session(&res)?;
  res.error_for_status_ref().context(format!("POST {url} returned error status"))?;
  res.text().context(format!("Failed to read body from POST {url}"))
}

fn post_form(path: &str, fields: &[(&str, &str)]) -> Result<String> {
  let url = format!("{BASE_URL}{path}");
  debug_log!("POST {url}");

  let csrf = fields.iter().find(|(k, _)| *k == "authenticity_token").map(|(_, v)| *v).unwrap_or("");

  let res = retry(Exponential::from_millis(500).map(jitter).take(3), || {
    client()?
      .post(&url)
      .header("X-Requested-With", "XMLHttpRequest")
      .header("Accept", "text/javascript, application/javascript, application/ecmascript, */*; q=0.01")
      .header("Origin", BASE_URL)
      .header("Referer", &url)
      .header("X-CSRF-Token", csrf)
      .form(fields)
      .send()
      .context(format!("POST {url} failed"))
  })
  .map_err(|e| e.error)?;

  // 302 redirect back to the book page is a success for StoryGraph mutations.
  if res.status() == StatusCode::FOUND {
    let location = res.headers().get("location").and_then(|v| v.to_str().ok()).unwrap_or("");
    if location.contains("/users/sign_in") {
      bail!(
        "StoryGraph session has expired. Please update <i>session_cookie</i> and <i>remember_user_token</i> in <i>.adds/NickelStorygraph/config.ini</i>."
      );
    }
    return Ok(String::new());
  }

  check_session(&res)?;
  res.error_for_status_ref().context(format!("POST {url} returned error status"))?;
  res.text().context(format!("Failed to read body from POST {url}"))
}

// ── HTML helpers ──────────────────────────────────────────────────────────────

fn csrf_token(html: &str) -> Option<String> {
  let doc = Html::parse_document(html);

  let meta_sel = Selector::parse(r#"meta[name="csrf-token"]"#).expect("valid selector");
  if let Some(el) = doc.select(&meta_sel).next() {
    if let Some(content) = el.value().attr("content") {
      return Some(content.to_owned());
    }
  }

  let input_sel = Selector::parse(r#"input[name="authenticity_token"]"#).expect("valid selector");
  doc
    .select(&input_sel)
    .next()
    .and_then(|el| el.value().attr("value"))
    .map(str::to_owned)
}

fn text_of<'a>(doc: &'a Html, selector: &str) -> String {
  Selector::parse(selector)
    .ok()
    .and_then(|sel| doc.select(&sel).next())
    .map(|el| el.text().collect::<String>().trim().to_owned())
    .unwrap_or_default()
}

fn extract_value_near_name(html: &str, needle: &str) -> Option<String> {
  let i = html.find(needle)?;
  let after = &html[i..];
  let v_pos = after.find("value=\"")?;
  // Ensure value= is within the same tag (no > between)
  if after[..v_pos].contains('>') { return None; }
  let start = v_pos + 7;
  let end = after[start..].find('"')?;
  Some(after[start..start + end].to_owned())
}

fn hidden_value(html: &str, name: &str) -> String {
  let name_needle = format!("name=\"{name}\"");
  // Try name=...value=... order first, then value=...name=... order.
  extract_value_near_name(html, &name_needle)
    .or_else(|| {
      // Reverse: find value= that appears just before name=
      let mut pos = 0;
      while let Some(i) = html[pos..].find("value=\"") {
        let abs = pos + i;
        let tag_start = html[..abs].rfind('<').unwrap_or(0);
        let tag_slice = &html[tag_start..abs + 100.min(html.len() - abs)];
        if tag_slice.contains(&name_needle) {
          let start = abs + 7;
          if let Some(end) = html[start..].find('"') {
            return Some(html[start..start + end].to_owned());
          }
        }
        pos = abs + 1;
      }
      None
    })
    .unwrap_or_default()
}

fn parse_status_text(text: &str) -> Option<u8> {
  let lower = text.to_lowercase();
  if lower.contains("rereading") {
    Some(6)
  } else if lower.contains("currently reading") {
    Some(2)
  } else if lower.contains("to-read") || lower.contains("to read") {
    Some(1)
  } else if lower.contains("paused") {
    Some(4)
  } else if lower.contains("did not finish") {
    Some(5)
  } else if lower.trim() == "read" || lower.ends_with(" read") || lower.starts_with("read ") {
    Some(3)
  } else {
    None
  }
}

fn status_to_str(status_id: u8) -> &'static str {
  match status_id {
    1 => "to-read",
    2 => "currently-reading",
    3 => "read",
    4 => "paused",
    5 => "did-not-finish",
    6 => "rereading",
    _ => "currently-reading",
  }
}

/// Fetch /books/:slug and extract mutable form state needed for POST calls.
fn fetch_page_data(book_id: &str) -> Result<(String, PageData)> {
  let html = get(&format!("/books/{book_id}"))?;
  let csrf = csrf_token(&html).context("Could not find CSRF token on book page")?;
  let book_pages = hidden_value(&html, "read_status[book_num_of_pages]");
  let last_pages = hidden_value(&html, "read_status[last_reached_pages]");
  let last_percent = hidden_value(&html, "read_status[last_reached_percent]");

  Ok((html, PageData { csrf, book_pages, last_pages, last_percent }))
}

// ── Public API ────────────────────────────────────────────────────────────────

/// A single reading goal (books, pages, or hours) from the challenges dashboard.
#[derive(Serialize, Debug)]
pub struct GoalEntry {
  pub label: String,    // "Books", "Pages", "Hours"
  pub current: String,  // raw string: "10", "2,458", "27.98"
  pub target: String,   // raw string: "26", "10,000", "50"
  pub unit: String,     // "books", "pages", "hours"
  pub percent: u32,     // 0–100
  pub status: String,   // status message text
}

/// Reading goals summary from the StoryGraph challenges dashboard.
#[derive(Serialize, Debug)]
pub struct ReadingGoals {
  pub year: String,
  pub books_read: u32,
  pub goals: Vec<GoalEntry>,
}

/// Fetches the user's reading goals from `/reading_challenges/dashboard/{slug}`.
pub fn get_goals() -> Result<ReadingGoals> {
  assert_credentials();
  let user = get_user()?;
  let html = get(&format!("/reading_challenges/dashboard/{}", user.slug))?;
  let doc = Html::parse_document(&html);

  // Find a 4-digit "20xx" year in any heading; fall back to the current calendar year.
  let heading_sel = Selector::parse("h1, h2, h3").expect("valid");
  let year = doc
    .select(&heading_sel)
    .find_map(|el| {
      el.text()
        .collect::<String>()
        .split_whitespace()
        .find(|w| w.len() == 4 && w.starts_with("20") && w.chars().all(|c| c.is_ascii_digit()))
        .map(str::to_owned)
    })
    .unwrap_or_else(|| {
      use chrono::Datelike;
      chrono::Local::now().year().to_string()
    });

  // "You have read 10 books this year." — count comes from the link text
  let books_link_sel = Selector::parse(r#"a[href*="/books-read/"]"#).expect("valid");
  let books_read: u32 = doc
    .select(&books_link_sel)
    .next()
    .and_then(|el| {
      el.text()
        .collect::<String>()
        .split_whitespace()
        .next()
        .and_then(|s| s.parse().ok())
    })
    .unwrap_or(0);

  // h6 labels ("Books", "Pages", "Hours") — each appears twice (mobile + desktop); deduplicate
  let h6_sel = Selector::parse("h6").expect("valid");
  let mut labels: Vec<String> = Vec::new();
  let mut seen_labels = std::collections::HashSet::new();
  for el in doc.select(&h6_sel) {
    let text = el
      .text()
      .find(|s| !s.trim().is_empty())
      .map(|s| s.trim().to_owned())
      .unwrap_or_default();
    if !text.is_empty() && seen_labels.insert(text.clone()) {
      labels.push(text);
    }
  }

  // Mobile count paragraphs: "10/26 books", "2,458/10,000 pages", "27.98/50 hours"
  let count_sel = Selector::parse("p.flex-shrink-0").expect("valid");
  let counts: Vec<String> = doc
    .select(&count_sel)
    .map(|el| el.text().collect::<String>().trim().to_owned())
    .collect();

  // Progress fill div style="width:X%" — one per goal in DOM order
  let fill_sel = Selector::parse(r#"div[style*="width:"]"#).expect("valid");
  let percents: Vec<u32> = doc
    .select(&fill_sel)
    .filter_map(|el| {
      let style = el.value().attr("style")?;
      let i = style.find("width:")?;
      let after = style[i + 6..].trim_start();
      let end = after.find('%')?;
      after[..end].trim().parse().ok()
    })
    .collect();

  // Status messages from italic mobile divs (shorter single-line versions)
  let status_sel = Selector::parse("div.italic p").expect("valid");
  let statuses: Vec<String> = doc
    .select(&status_sel)
    .map(|el| el.text().collect::<String>().trim().to_owned())
    .collect();

  debug_log!(
    "get_goals(): year={:?}, books_read={}, labels={:?}, counts={:?}, percents={:?}, statuses={:?}",
    year,
    books_read,
    labels,
    counts,
    percents,
    statuses
  );

  let n = labels.len().min(counts.len()).min(percents.len());
  let mut goals = Vec::new();
  for i in 0..n {
    let label = labels[i].clone();
    let percent = percents[i];
    let status = statuses.get(i).cloned().unwrap_or_default();

    // "10/26 books" → current="10", target="26", unit="books"
    let (current, target, unit) = if let Some((left, right)) = counts[i].split_once('/') {
      let current = left.trim().to_owned();
      let mut parts = right.trim().splitn(2, ' ');
      let target = parts.next().unwrap_or("").to_owned();
      let unit = parts.next().unwrap_or("").to_owned();
      (current, target, unit)
    } else {
      (String::new(), String::new(), String::new())
    };

    goals.push(GoalEntry { label, current, target, unit, percent, status });
  }

  Ok(ReadingGoals { year, books_read, goals })
}

/// The user's current StoryGraph reading streak.
#[derive(Serialize, Debug, Default)]
pub struct Streak {
  pub requirement: String,  // e.g. "1 page, or 1 minute, every day"
  pub current: String,      // e.g. "1"
  pub longest: String,      // e.g. "46"
  pub period: String,       // e.g. "Jul 18"
  pub reading_left: String, // e.g. "Reading left: 1 page or 1 minute"
}

/// Fetches the user's reading streak from `/streak/{slug}`.
pub fn get_streak() -> Result<Streak> {
  assert_credentials();
  let user = get_user()?;
  let html = get(&format!("/streak/{}", user.slug))?;
  let doc = Html::parse_document(&html);

  // "font-medium" and bare "dl" are generic Tailwind utility classes/tags
  // that could easily match unrelated elements elsewhere on a real
  // (non-fragment) page, so scope searches as tightly as the known markup
  // allows rather than matching anywhere in the document.

  // The <p class="font-medium"> holding the requirement text is a direct
  // sibling of the "Streak" <h1> under the same parent div.
  let heading_sel = Selector::parse("h1.page-heading").expect("valid");
  let requirement_container = doc
    .select(&heading_sel)
    .find(|el| el.text().collect::<String>().trim() == "Streak")
    .and_then(|h1| h1.parent())
    .and_then(scraper::ElementRef::wrap);

  // The streak requirement ("1 page, or 1 minute, every day", "5 pages,
  // every day", etc.) is shown verbatim rather than parsed, since the
  // wording/units vary per user's streak settings.
  let requirement_sel = Selector::parse("p.font-medium span").expect("valid");
  let requirement = requirement_container
    .and_then(|c| c.select(&requirement_sel).next())
    .map(|el| el.text().collect::<String>().trim().to_owned())
    .unwrap_or_default();

  // Current/longest streak and the current-period note each live in a
  // dt/dd pair under a shared parent div inside the streak stats <dl>
  // (identified by its specific classes rather than just "dl") — walk
  // dt's parent to find its matching dd rather than assuming array
  // alignment across all pairs.
  let dt_sel = Selector::parse("dl.font-semibold dt").expect("valid");
  let dd_sel = Selector::parse("dl.font-semibold dd").expect("valid");
  let italic_sel = Selector::parse("span.italic").expect("valid");

  let mut streak = Streak { requirement, ..Default::default() };

  for dt in doc.select(&dt_sel) {
    let label = dt.text().collect::<String>().trim().to_owned();
    let Some(parent) = dt.parent().and_then(scraper::ElementRef::wrap) else { continue };
    let dd_text = parent
      .select(&dd_sel)
      .next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .unwrap_or_default();

    if label == "Current streak" {
      streak.current = dd_text;
    } else if label == "Longest streak" {
      streak.longest = dd_text;
    } else if label.starts_with("Current period") {
      streak.period = dt
        .select(&italic_sel)
        .next()
        .map(|el| el.text().collect::<String>().trim().to_owned())
        .unwrap_or_default();
      streak.reading_left = dd_text;
    }
  }

  debug_log!("get_streak(): {streak:?}");
  Ok(streak)
}

/// A book on the user's currently-reading or paused shelf.
#[derive(Serialize, Debug)]
pub struct ReadingBook {
  pub id: String,
  pub title: String,
  pub author: String,
  pub progress_percent: u32,
  pub pages: u32,
  pub format: String,
  pub year: u32,
  pub status: String,            // "reading" or "paused"
  pub cover_url: Option<String>, // absolute URL or None
}

/// A single item in the community activity feed.
#[derive(Serialize, Debug)]
pub struct FeedItem {
  pub username: String,
  pub action: String,               // "started reading", "finished reading and reviewed", etc.
  pub time_ago: String,             // "1d", "2h", "5m"
  pub book_title: String,
  pub book_author: String,
  pub cover_url: Option<String>,
  pub genres: Vec<String>,
  pub moods: Vec<String>,
  pub rating: Option<f32>,          // present on "finished reading and reviewed" items
  pub review_snippet: Option<String>, // truncated review text, if present
}

/// Fetches the community activity feed from `/community`.
/// Items live inside `.news-feed-item-panes` as `[data-news-feed-item-id]` elements.
pub fn get_feed() -> Result<Vec<FeedItem>> {
  assert_credentials();
  let html = get("/community")?;
  let doc = Html::parse_document(&html);

  let item_sel    = Selector::parse("[data-news-feed-item-id]").expect("valid");
  let h3_sel      = Selector::parse("h3").expect("valid");
  let profile_sel = Selector::parse(r#"a[href^="/profile/"]"#).expect("valid");
  let cover_sel   = Selector::parse(".book-cover img").expect("valid");
  let title_sel   = Selector::parse("h4 a").expect("valid");
  let author_sel  = Selector::parse(r#"p a[href^="/authors/"]"#).expect("valid");
  let time_sel    = Selector::parse(r#"span[class*="absolute"]"#).expect("valid");
  let teal_sel    = Selector::parse(r#"span[class*="text-teal-800"]"#).expect("valid");
  let pink_sel    = Selector::parse(r#"span[class*="text-pink-700"]"#).expect("valid");
  let rating_sel  = Selector::parse(r#"a[aria-label*="Rating:"]"#).expect("valid");
  let review_sel  = Selector::parse(r#"a[class*="clamp-3"]"#).expect("valid");

  let mut items = Vec::new();

  for item in doc.select(&item_sel) {
    // Time: the absolute-positioned span in the top-right corner
    let time_ago = item.select(&time_sel).next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .unwrap_or_default();

    // Username + action from the first h3 (mobile header)
    let (username, action) = if let Some(h3) = item.select(&h3_sel).next() {
      let username = h3.select(&profile_sel).next()
        .map(|el| el.text().collect::<String>().trim().to_owned())
        .unwrap_or_default();

      // Collect all text nodes; strip username prefix to get the action verb
      let full: String = h3.text().map(|t| t.trim()).filter(|t| !t.is_empty()).collect::<Vec<_>>().join(" ");
      let action = if username.is_empty() {
        full.trim().to_owned()
      } else {
        full.trim()
          .replacen(&username, "", 1)
          .trim()
          .trim_end_matches(':')
          .trim()
          .to_owned()
      };
      (username, action)
    } else {
      (String::new(), String::new())
    };

    if username.is_empty() { continue; }

    // Book cover
    let cover_url = item.select(&cover_sel).next()
      .and_then(|el| el.value().attr("src").or_else(|| el.value().attr("data-src")))
      .map(|src| {
        if src.starts_with("http") { src.to_owned() }
        else { format!("{BASE_URL}{src}") }
      });

    let book_title = item.select(&title_sel).next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .unwrap_or_default();

    let book_author = item.select(&author_sel).next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .unwrap_or_default();

    // Genres (teal) and moods (pink) — deduplicated (each tag appears twice in mobile+desktop)
    let mut seen_g = std::collections::HashSet::new();
    let genres: Vec<String> = item.select(&teal_sel)
      .filter_map(|el| {
        let t = el.text().collect::<String>().trim().to_owned();
        if t.is_empty() || !seen_g.insert(t.clone()) { None } else { Some(t) }
      })
      .collect();

    let mut seen_m = std::collections::HashSet::new();
    let moods: Vec<String> = item.select(&pink_sel)
      .filter_map(|el| {
        let t = el.text().collect::<String>().trim().to_owned();
        if t.is_empty() || !seen_m.insert(t.clone()) { None } else { Some(t) }
      })
      .collect();

    // Rating: from aria-label="Rating: 2.0 out of 5"
    let rating = item.select(&rating_sel).next()
      .and_then(|el| el.value().attr("aria-label"))
      .and_then(|label| {
        label.strip_prefix("Rating: ")
          .and_then(|s| s.split(" out of").next())
          .and_then(|s| s.trim().parse::<f32>().ok())
      });

    // Review snippet from line-clamped link
    let review_snippet = item.select(&review_sel).next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .filter(|s| !s.is_empty());

    items.push(FeedItem { username, action, time_ago, book_title, book_author, cover_url, genres, moods, rating, review_snippet });
  }

  debug_log!("get_feed(): {} items", items.len());
  Ok(items)
}

fn scrape_shelf_section(doc: &Html, container_selector: &str, status: &str) -> Vec<ReadingBook> {
  let container_sel = Selector::parse(container_selector).expect("valid");
  let pane_sel      = Selector::parse(".book-pane").expect("valid");
  let title_sel     = Selector::parse("h3.font-bold a").expect("valid");
  let author_sel    = Selector::parse(r#"a[href^="/authors/"]"#).expect("valid");
  let progress_sel  = Selector::parse("input.read-status-last-reached-percent").expect("valid");
  let edition_sel   = Selector::parse(".toggle-edition-info-link").expect("valid");
  let cover_sel     = Selector::parse(".book-cover img").expect("valid");

  let mut books = Vec::new();

  for container in doc.select(&container_sel) {
    for pane in container.select(&pane_sel) {
      let book_id = match pane.value().attr("data-book-id") {
        Some(id) if !id.is_empty() => id.to_owned(),
        _ => continue,
      };

      let title = pane
        .select(&title_sel)
        .next()
        .map(|el| el.text().collect::<String>().trim().to_owned())
        .unwrap_or_default();

      if title.is_empty() {
        continue;
      }

      let author = pane
        .select(&author_sel)
        .next()
        .map(|el| el.text().collect::<String>().trim().to_owned())
        .unwrap_or_default();

      let progress: u32 = pane
        .select(&progress_sel)
        .next()
        .and_then(|el| el.value().attr("value"))
        .and_then(|v| v.parse().ok())
        .unwrap_or(0);

      let edition_text = pane
        .select(&edition_sel)
        .next()
        .map(|el| el.text().collect::<String>())
        .unwrap_or_default();

      let parts: Vec<&str> = edition_text.split(" \u{2022} ").collect();
      let pages: u32 = parts
        .first()
        .and_then(|s| s.trim().strip_suffix(" pages"))
        .and_then(|n| n.trim().parse().ok())
        .unwrap_or(0);
      let format = parts.get(1).map(|s| s.trim().to_owned()).unwrap_or_default();
      let year: u32 = parts.get(2).and_then(|s| s.trim().parse().ok()).unwrap_or(0);

      let cover_url = pane.select(&cover_sel).next()
        .and_then(|el| el.value().attr("src").or_else(|| el.value().attr("data-src")))
        .map(|src| {
          if src.starts_with("http") { src.to_owned() }
          else { format!("{BASE_URL}{src}") }
        });

      books.push(ReadingBook {
        id: book_id,
        title,
        author,
        progress_percent: progress,
        pages,
        format,
        year,
        status: status.to_owned(),
        cover_url,
      });
    }
  }

  books
}

/// Returns books on the currently-reading shelf followed by books on the paused shelf.
/// Both sections live on the same `/currently-reading/{slug}` page:
///   .read-books         → currently reading
///   .paused-books-panes → paused
pub fn list_reading() -> Result<Vec<ReadingBook>> {
  assert_credentials();
  let user = get_user()?;
  let html = get(&format!("/currently-reading/{}", user.slug))?;
  let doc = Html::parse_document(&html);

  let mut books = scrape_shelf_section(&doc, ".read-books", "reading");
  books.extend(scrape_shelf_section(&doc, ".paused-books-panes", "paused"));

  debug_log!("list_reading(): {} books total", books.len());
  Ok(books)
}

/// Returns the logged-in user's slug. Uses `user_slug` from config if set,
/// otherwise scrapes the homepage nav for a `/users/` profile link.
pub fn get_user() -> Result<User> {
  assert_credentials();

  if !CONFIG.user_slug.is_empty() {
    return Ok(User { username: CONFIG.user_slug.clone(), slug: CONFIG.user_slug.clone() });
  }

  let html = get("/")?;
  let doc = Html::parse_document(&html);

  // The nav has a link to the user's profile: /users/:slug
  let sel = Selector::parse(r#"a[href^="/users/"]"#).expect("valid selector");
  let slug = doc
    .select(&sel)
    .filter_map(|el| el.value().attr("href"))
    .filter(|href| !href.contains("/sign_in") && !href.contains("/sign_up"))
    .filter_map(|href| href.strip_prefix("/users/"))
    .map(str::to_owned)
    .next()
    .context("Could not find user profile link — session may be expired")?;

  Ok(User { username: slug.clone(), slug })
}

/// Searches StoryGraph by title/author/ISBN and returns matching books.
/// Audio editions are filtered out. Cover URLs are not populated (StoryGraph
/// serves covers from a CDN that requires a separate request).
pub fn search(query: &str) -> Result<Vec<Book>> {
  assert_credentials();
  let html = get(&format!("/browse?search_term={}", urlencoding::encode(query)))?;
  let doc = Html::parse_document(&html);

  let container_sel = Selector::parse(".book-title-author-and-series").expect("valid selector");
  let title_sel = Selector::parse(r#"a[href^="/books/"]"#).expect("valid selector");
  let author_sel = Selector::parse(r#"a[href^="/authors/"]"#).expect("valid selector");

  let mut results = Vec::new();
  let mut seen = std::collections::HashSet::new();

  for container in doc.select(&container_sel) {
    let container_html = container.html();
    let container_doc = Html::parse_fragment(&container_html);

    let title_el = container_doc.select(&title_sel).next();
    if let Some(title_el) = title_el {
      let book_id = title_el
        .value()
        .attr("href")
        .and_then(|href| href.strip_prefix("/books/"))
        .map(str::to_owned);

      if let Some(book_id) = book_id {
        if seen.contains(&book_id) {
          continue;
        }

        // Skip audio editions based on format info in nearby text
        let pane_text = container_html.to_lowercase();
        if pane_text.contains("format: audio") {
          continue;
        }

        let title = title_el.text().collect::<String>().trim().to_owned();
        let author = container_doc
          .select(&author_sel)
          .next()
          .map(|el| el.text().collect::<String>().trim().to_owned())
          .unwrap_or_else(|| "Unknown Author".to_owned());

        // Page count from "X pages" text nearby
        let pages = pane_text
          .split_whitespace()
          .zip(pane_text.split_whitespace().skip(1))
          .find(|(_, next)| *next == "pages")
          .and_then(|(n, _)| n.parse::<u32>().ok())
          .unwrap_or(0);

        seen.insert(book_id.clone());
        results.push(Book {
          book_id,
          title,
          author,
          cover_url: None,
          pages,
        });
      }
    }
  }

  debug_log!("search({query:?}) → {} results", results.len());
  Ok(results)
}

/// Search by ISBN — returns the first non-audio match, or None.
pub fn find_by_isbn(isbn: &str) -> Result<Option<Book>> {
  let results = search(isbn)?;
  Ok(results.into_iter().next())
}

/// Fetches the current user's reading status and progress for a book.
/// Returns zeroed progress fields if the book has no page count on StoryGraph
/// (common for Kindle/digital editions).
pub fn get_user_book(book_id: &str) -> Result<UserBook> {
  assert_credentials();
  let html = get(&format!("/books/{book_id}"))?;
  parse_user_book(book_id, &html)
}

fn parse_user_book(book_id: &str, html: &str) -> Result<UserBook> {
  let doc = Html::parse_document(html);

  let status_text = text_of(&doc, ".read-status-label");
  let status_id = parse_status_text(&status_text);

  // Try progress bar width first (most reliable), then hidden inputs.
  let progress_percent = html
    .find("edit-progress")
    .and_then(|i| {
      let s = &html[i..];
      let w = s.find("width:")?;
      let pct = s[w + 6..].trim_start();
      let end = pct.find('%')?;
      pct[..end].trim().parse::<u32>().ok()
    })
    .unwrap_or_else(|| hidden_value(html, "read_status[last_reached_percent]").parse().unwrap_or(0));

  let progress_pages: u32 = hidden_value(html, "read_status[last_reached_pages]")
    .parse()
    .unwrap_or_else(|_| extract_value_near_name(html, "read-status-last-reached-pages").and_then(|v| v.parse().ok()).unwrap_or(0));
  let book_pages: u32 = hidden_value(html, "read_status[book_num_of_pages]")
    .parse()
    .unwrap_or_else(|_| extract_value_near_name(html, "read-status-book-num-of-pages").and_then(|v| v.parse().ok()).unwrap_or(0));

  let progress_type = Selector::parse(".read-status-progress-type option[selected]")
    .ok()
    .and_then(|sel| doc.select(&sel).next())
    .and_then(|el| el.value().attr("value"))
    .unwrap_or("percentage")
    .to_owned();

  let edition_format = {
    let sel = Selector::parse(".edition-info p").expect("valid selector");
    doc
      .select(&sel)
      .find(|el| el.text().collect::<String>().contains("Format:"))
      .map(|el| {
        el.text()
          .collect::<String>()
          .replacen("Format:", "", 1)
          .trim()
          .to_owned()
      })
      .unwrap_or_default()
  };

  Ok(UserBook {
    book_id: book_id.to_owned(),
    status_id,
    progress_percent,
    progress_pages,
    book_pages,
    progress_type,
    edition_format,
  })
}

/// Sets the reading status of a book (to-read, reading, read, paused, dnf).
/// POSTs to `/update-status.js` and re-fetches the book page to confirm.
pub fn update_status(book_id: &str, status_id: u8) -> Result<UserBook> {
  assert_credentials();
  let (_html, data) = fetch_page_data(book_id)?;
  let status_str = status_to_str(status_id);

  let fields = vec![
    ("authenticity_token", data.csrf.as_str()),
    ("book_id", book_id),
    ("status", status_str),
  ];

  let path = format!("/update-status.js?book_id={book_id}&status={status_str}");
  post_form(&path, &fields[..1])?;

  // Re-fetch to return current state.
  parse_user_book(book_id, &get(&format!("/books/{book_id}"))?)
}

/// Updates reading progress for a book. `progress_type` is `"percentage"` or `"pages"`.
/// Fetches the book page first to extract the CSRF token and current page count.
/// If the book has no reading status, automatically sets it to "Currently Reading" first.
pub fn update_progress(book_id: &str, progress_type: &str, value: u32) -> Result<UserBook> {
  assert_credentials();
  let (html, mut data) = fetch_page_data(book_id)?;

  let current_status = parse_user_book(book_id, &html)?.status_id;
  if current_status.is_none() {
    update_status(book_id, 2)?;
    let (_, fresh) = fetch_page_data(book_id)?;
    data = fresh;
  } else if current_status == Some(3) {
    update_status(book_id, 6)?;
    let (_, fresh) = fetch_page_data(book_id)?;
    data = fresh;
  }

  let value_str = value.to_string();

  post_form("/update-progress", &[
    ("authenticity_token", &data.csrf),
    ("read_status[progress_number]", &value_str),
    ("read_status[progress_type]", progress_type),
    ("read_status[book_num_of_pages]", &data.book_pages),
    ("book_id", book_id),
    ("on_book_page", "true"),
  ])?;

  parse_user_book(book_id, &get(&format!("/books/{book_id}"))?)
}

/// Creates a reading journal entry (note + progress snapshot) via `/update-progress-with-note`.
/// StoryGraph records this as a progress update with an attached note in the reading journal.
pub fn insert_journal(book_id: &str, note: &str, progress: u32, progress_type: &str) -> Result<()> {
  assert_credentials();
  let (_html, data) = fetch_page_data(book_id)?;
  let progress_str = progress.to_string();

  let now = chrono::Local::now();
  let day = now.format("%d").to_string();
  let month = now.format("%m").to_string();
  let year = now.format("%Y").to_string();

  post_form("/update-progress-with-note", &[
    ("authenticity_token", &data.csrf),
    ("progress_update_date[day]", &day),
    ("progress_update_date[month]", &month),
    ("progress_update_date[year]", &year),
    ("progress_minutes", ""),
    ("progress_number", &progress_str),
    ("progress_type", progress_type),
    ("last_reached_pages", &data.last_pages),
    ("book_num_of_pages", &data.book_pages),
    ("last_reached_percent", &data.last_percent),
    ("note", note),
    ("book_id", book_id),
    ("return_to", ""),
    ("button", ""),
  ])?;

  Ok(())
}

/// Submits a new review to StoryGraph via POST /reviews.
/// Rating is split: 4.75 → stars_integer=4, stars_decimal=75.
/// mood_ids are the StoryGraph integer IDs 1–14.
pub fn submit_review(
  book_id: &str,
  rating: Option<f32>,
  explanation: &str,
  mood_ids: &[u8],
  pace: &str,
  character_or_plot_driven: &str,
  strong_character_development: &str,
  loveable_characters: &str,
  diverse_characters: &str,
  flawed_characters: &str,
) -> Result<()> {
  assert_credentials();

  let book_uuid = book_id;

  // Fetch /reviews/new for a CSRF token scoped to the review form.
  let form_html = get(&format!("/reviews/new?book_id={book_uuid}&return_to=%2Fbooks%2F{book_uuid}"))?;
  let csrf = csrf_token(&form_html).context("Could not find CSRF token on reviews/new page")?;

  let (stars_int, stars_dec) = match rating {
    Some(r) if r > 0.0 => {
      let int_part = r.floor() as u8;
      let frac = r - int_part as f32;
      let dec = if frac < 0.13 { String::new() } else if frac < 0.38 { "25".into() } else if frac < 0.63 { "5".into() } else { "75".into() };
      (int_part.to_string(), dec)
    }
    _ => (String::new(), String::new()),
  };

  let explanation_html = if explanation.is_empty() {
    String::new()
  } else if explanation.starts_with('<') {
    explanation.to_owned()
  } else {
    format!("<div>{}</div>", explanation)
  };

  let return_to = format!("/books/{book_uuid}");
  let mut fields: Vec<(String, String)> = vec![
    ("authenticity_token".into(), csrf),
    ("stars_integer".into(), stars_int),
    ("stars_decimal".into(), stars_dec),
    ("review[explanation]".into(), explanation_html),
    ("review[book_id]".into(), book_uuid.to_string()),
    ("return_to".into(), return_to),
    ("review[pace]".into(), pace.to_owned()),
    ("review[character_or_plot_driven]".into(), character_or_plot_driven.to_owned()),
    ("review[strong_character_development]".into(), strong_character_development.to_owned()),
    ("review[loveable_characters]".into(), loveable_characters.to_owned()),
    ("review[diverse_characters]".into(), diverse_characters.to_owned()),
    ("review[flawed_characters]".into(), flawed_characters.to_owned()),
    ("review[themes]".into(), String::new()),
    ("review[content_warning_description]".into(), String::new()),
    ("button".into(), String::new()),
  ];
  for &id in mood_ids {
    fields.push(("review[mood_ids][]".into(), id.to_string()));
  }

  post_form_browser("/reviews", &fields)?;
  Ok(())
}

/// Returns all non-audio editions of a book from its `/editions` page.
/// Used by the hook's edition picker so the user can switch to the correct
/// physical or digital edition for accurate page tracking.
pub fn list_editions(book_id: &str, reading_format: i32, language: &str) -> Result<Vec<Edition>> {
  assert_credentials();
  let html = get(&format!("/books/{book_id}/editions"))?;
  let doc = Html::parse_document(&html);

  // Diagnostic: log what selectors actually hit so we can tune if the page structure differs.
  let pane_sel = Selector::parse(".book-pane").expect("valid selector");
  let title_container_sel = Selector::parse(".book-title-author-and-series").expect("valid selector");
  let pane_count = doc.select(&pane_sel).count();
  let title_count = doc.select(&title_container_sel).count();
  debug_log!(
    "list_editions({book_id}): HTML len={}, has 'book-pane'={}, has 'data-book-id'={}, .book-pane count={pane_count}, .book-title-author-and-series count={title_count}",
    html.len(),
    html.contains("book-pane"),
    html.contains("data-book-id"),
  );

  let info_sel = Selector::parse(".edition-info p").expect("valid selector");
  let img_sel = Selector::parse(".book-cover img").expect("valid selector");
  let title_sel = Selector::parse(".book-title-author-and-series h3 a").expect("valid selector");
  let link_sel = Selector::parse(r#"a[href^="/books/"]"#).expect("valid selector");

  let mut editions = Vec::new();

  for pane in doc.select(&pane_sel) {
    // Prefer data-book-id attribute; fall back to first /books/ link in the pane.
    let book_id = pane
      .value()
      .attr("data-book-id")
      .map(str::to_owned)
      .or_else(|| {
        pane
          .select(&link_sel)
          .filter_map(|el| el.value().attr("href"))
          .filter_map(|href| href.strip_prefix("/books/"))
          .map(str::to_owned)
          .next()
      });

    let book_id = match book_id {
      Some(id) => id,
      None => {
        debug_log!("list_editions: skipping pane with no book id");
        continue;
      }
    };

    let title = pane
      .select(&title_sel)
      .next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .unwrap_or_default();

    let mut isbn = String::new();
    let mut format = String::new();
    let mut language = String::new();
    let mut publisher = String::new();
    let mut pub_date = String::new();
    let mut pages: Option<u32> = None;

    for p in pane.select(&info_sel) {
      let text = p.text().collect::<String>();
      if text.contains("ISBN/UID:") {
        isbn = text.replacen("ISBN/UID:", "", 1).trim().to_owned();
      } else if text.contains("Format:") {
        format = text.replacen("Format:", "", 1).trim().to_owned();
      } else if text.contains("Language:") {
        language = text.replacen("Language:", "", 1).trim().to_owned();
      } else if text.contains("Publisher:") {
        publisher = text.replacen("Publisher:", "", 1).trim().to_owned();
      } else if text.contains("Edition Pub Date:") {
        pub_date = text.replacen("Edition Pub Date:", "", 1).trim().to_owned();
      }
    }

    // Page count from "X pages" text
    let pane_text = pane.text().collect::<String>();
    for (a, b) in pane_text.split_whitespace().zip(pane_text.split_whitespace().skip(1)) {
      if b == "pages" {
        if let Ok(n) = a.parse::<u32>() {
          pages = Some(n);
          break;
        }
      }
    }

    // Skip audio editions
    if format.to_lowercase().contains("audio") {
      continue;
    }

    let cover_url = pane
      .select(&img_sel)
      .next()
      .and_then(|el| el.value().attr("src"))
      .map(str::to_owned);

    editions.push(Edition {
      book_id,
      title,
      format,
      pages,
      isbn,
      language,
      publisher,
      pub_date,
      cover_url,
    });
  }

  let is_digital = |f: &str| {
    let f = f.to_lowercase();
    f.contains("ebook") || f.contains("e-book") || f.contains("epub")
      || f.contains("kindle") || f.contains("digital") || f.contains("mobi")
      || f.contains("pdf")
  };

  let editions: Vec<Edition> = editions
    .into_iter()
    .filter(|e| match reading_format {
      1 => !is_digital(&e.format), // Print
      4 => is_digital(&e.format),  // E-Book
      _ => true,                   // All
    })
    .filter(|e| language.is_empty() || e.language.eq_ignore_ascii_case(language))
    .collect();

  debug_log!("list_editions({book_id}): returning {} editions (format={reading_format}, lang={language:?})", editions.len());
  Ok(editions)
}

/// Returns what fraction of the meaningful words in `query_title` appear in `result_title`.
pub fn title_match_score(query_title: &str, result_title: &str) -> f32 {
  const STOPS: &[&str] = &["the", "a", "an", "of", "in", "by", "to", "and", "or", "for", "at", "its", "is"];

  let tokenize = |s: &str| -> Vec<String> {
    s.to_lowercase()
      .split(|c: char| !c.is_alphabetic())
      .filter(|w| w.len() > 1 && !STOPS.contains(w))
      .map(str::to_owned)
      .collect()
  };

  let query_words = tokenize(query_title);
  if query_words.is_empty() {
    return 0.0;
  }

  let result_words: std::collections::HashSet<String> = tokenize(result_title).into_iter().collect();
  let matches = query_words.iter().filter(|w| result_words.contains(*w)).count();
  matches as f32 / query_words.len() as f32
}

/// Searches the user's currently-reading shelf for a book matching `query_title` without
/// needing a book_id first. Parses shelf HTML once; returns `(book_id, progress_percent)`
/// for the best title match (score ≥ 0.5), or `None` if nothing matches.
pub fn find_on_shelf(query_title: &str) -> Result<Option<(String, u32)>> {
  assert_credentials();
  let user = get_user()?;
  let html = get(&format!("/currently-reading/{}", user.slug))?;
  let doc = Html::parse_document(&html);

  let pane_sel     = Selector::parse(".book-pane").expect("valid");
  let title_sel    = Selector::parse("h3.font-bold a").expect("valid");
  let progress_sel = Selector::parse("input.read-status-last-reached-percent").expect("valid");

  let mut best: Option<(String, f32, u32)> = None;

  for pane in doc.select(&pane_sel) {
    let book_id = match pane.value().attr("data-book-id") {
      Some(id) if !id.is_empty() => id.to_owned(),
      _ => continue,
    };

    let title: String = pane
      .select(&title_sel)
      .next()
      .map(|el| el.text().collect::<String>().trim().to_owned())
      .unwrap_or_default();

    if title.is_empty() {
      continue;
    }

    let score = title_match_score(query_title, &title);
    if score >= 0.5 {
      let progress: u32 = pane
        .select(&progress_sel)
        .next()
        .and_then(|el| el.value().attr("value"))
        .and_then(|v| v.parse().ok())
        .unwrap_or(0);

      if best.as_ref().map_or(true, |(_, s, _)| score > *s) {
        best = Some((book_id, score, progress));
      }
    }
  }

  debug_log!("find_on_shelf({query_title:?}): {:?}", best.as_ref().map(|(id, s, p)| (id, s, p)));
  Ok(best.map(|(id, _, progress)| (id, progress)))
}

/// Finds which edition of this book the user currently has on their StoryGraph
/// currently-reading shelf. Fetches the shelf page and looks for any edition UUID
/// that matches a known edition of the same work.
pub fn find_current_edition(book_id: &str) -> Result<String> {
  assert_credentials();

  let editions = list_editions(book_id, 0, "")?;
  let user = get_user()?;
  let shelf_html = get(&format!("/currently-reading/{}", user.slug))?;

  // Check the stored book_id first — it may already be the right one.
  if shelf_html.contains(book_id) {
    return Ok(book_id.to_owned());
  }

  // Check each edition UUID against the shelf HTML.
  for edition in &editions {
    if shelf_html.contains(&edition.book_id) {
      return Ok(edition.book_id.clone());
    }
  }

  anyhow::bail!("Could not find this book on your currently reading shelf on StoryGraph.")
}

/// If `book_id` is not a digital edition, returns the book_id of a digital edition
/// of the same work. Falls back to the original book_id if no digital edition is found.
pub fn prefer_digital_edition(book_id: &str) -> Result<String> {
  assert_credentials();
  let html = get(&format!("/books/{book_id}"))?;
  let user_book = parse_user_book(book_id, &html)?;

  let fmt = user_book.edition_format.to_lowercase();
  if fmt.is_empty() || fmt.contains("digital") {
    return Ok(book_id.to_owned());
  }

  let editions = list_editions(book_id, 0, "")?;
  Ok(editions
    .into_iter()
    .find(|e| e.format.to_lowercase().contains("digital"))
    .map(|e| e.book_id)
    .unwrap_or_else(|| book_id.to_owned()))
}

/// Switches the user's linked edition from `from_id` to `to_id`.
/// Finds the switch form on the editions page and POSTs it, so StoryGraph
/// transfers reading progress to the new edition.
pub fn switch_edition(from_id: &str, to_id: &str) -> Result<bool> {
  assert_credentials();
  let html = get(&format!("/books/{from_id}/editions"))?;
  let doc = Html::parse_document(&html);
  let csrf = csrf_token(&html).context("Could not find CSRF token on editions page")?;

  // Find the switch form for the target edition.
  let form_sel = Selector::parse("form[action='/switch-editions']").expect("valid selector");
  let input_sel = Selector::parse("input").expect("valid selector");

  let mut form_csrf = csrf.clone();
  let mut true_from_id = from_id.to_owned();

  for form in doc.select(&form_sel) {
    let inputs: std::collections::HashMap<String, String> = form
      .select(&input_sel)
      .filter_map(|el| {
        let name = el.value().attr("name")?.to_owned();
        let value = el.value().attr("value").unwrap_or("").to_owned();
        Some((name, value))
      })
      .collect();

    if inputs.get("to_book_id").map(String::as_str) == Some(to_id) {
      if let Some(v) = inputs.get("authenticity_token") {
        form_csrf = v.clone();
      }
      if let Some(v) = inputs.get("from_book_id") {
        true_from_id = v.clone();
      }
      break;
    }
  }

  post_form("/switch-editions", &[
    ("authenticity_token", &form_csrf),
    ("from_book_id", &true_from_id),
    ("to_book_id", to_id),
    ("button", ""),
  ])?;

  Ok(true)
}

/// Fetches the reading journal entries for a book from `/journal?book_id={uuid}`.
/// StoryGraph uses UUIDs (not slugs) in /books/ URLs, so book_id is already the UUID
/// that the journal endpoint expects.
pub fn list_journal(book_id: &str, limit: usize, offset: usize) -> Result<Vec<JournalActivity>> {
  assert_credentials();

  let html = get(&format!("/journal?book_id={book_id}"))?;
  let doc = Html::parse_document(&html);

  let entry_sel      = Selector::parse(".journal-entry-panes > div").expect("valid selector");
  let date_sel       = Selector::parse("p.font-semibold").expect("valid selector");
  let badge_sel      = Selector::parse("span[title]").expect("valid selector");
  let progress_sel   = Selector::parse(r#"div[style*="width:"]"#).expect("valid selector");
  let trix_sel       = Selector::parse("div.trix-content").expect("valid selector");
  let blockquote_sel = Selector::parse("blockquote").expect("valid selector");
  let note_div_sel   = Selector::parse("div.trix-content > div:not(.trix-content)").expect("valid selector");

  let all_entries: Vec<_> = doc.select(&entry_sel).collect();
  debug_log!("list_journal({book_id}): HTML len={}, found {} entries", html.len(), all_entries.len());

  let mut activities = Vec::new();
  let mut prev_progress: u32 = 0;

  for el in all_entries.iter().skip(offset).take(limit) {
    let date_text = el
      .select(&date_sel)
      .next()
      .and_then(|p| p.text().next())
      .map(|t| t.trim().to_owned())
      .unwrap_or_default();
    let action_at = parse_storygraph_date(&date_text);

    // Status badge ("Started reading", "Want to read", etc.)
    if let Some(badge) = el.select(&badge_sel).next() {
      let title = badge.value().attr("title").unwrap_or("").to_lowercase();
      let event = if title.contains("start") {
        "user_book_read_started"
      } else if title.contains("finish") {
        "user_book_read_finished"
      } else if title.contains("want to read") {
        "status_want_to_read"
      } else if title.contains("stop") || title.contains("dnf") || title.contains("did not finish") {
        "status_stopped"
      } else {
        "status_read"
      };
      activities.push(JournalActivity {
        event: event.to_owned(),
        action_at,
        entry: String::new(),
        metadata: Value::Null,
      });
      continue;
    }

    // Progress percentage from progress bar width style
    let progress: u32 = el
      .select(&progress_sel)
      .next()
      .and_then(|div| div.value().attr("style"))
      .and_then(|style| {
        let i = style.find("width:")?;
        let after = style[i + 6..].trim_start();
        let end = after.find('%')?;
        after[..end].trim().parse::<f32>().ok()
      })
      .map(|f| f.round() as u32)
      .unwrap_or(0);

    // Text content from the inner trix editor div.
    // If all the text lives inside <blockquote> descendants, it's a book quote.
    // We compare flat text of the whole trix div vs flat text of blockquote
    // descendants — equal means everything is quoted.
    let (entry_text, all_blockquote, quote_text) = if let Some(trix) = el.select(&trix_sel).next() {
      let flat: String = trix.text()
        .map(|t| t.trim())
        .filter(|t| !t.is_empty())
        .collect::<Vec<_>>()
        .join(" ");

      if flat.is_empty() {
        (String::new(), false, None)
      } else {
        let bq: String = trix.select(&blockquote_sel)
          .flat_map(|b| b.text())
          .map(|t| t.trim())
          .filter(|t| !t.is_empty())
          .collect::<Vec<_>>()
          .join(" ");

        if bq.is_empty() {
          // Pure note, no blockquote
          (flat, false, None)
        } else if bq == flat {
          // Everything is a blockquote — pure quote
          (bq, true, None)
        } else {
          // Mixed: blockquote + annotation — put quote in metadata, annotation in entry
          let non_bq: String = trix.select(&note_div_sel)
            .flat_map(|d| d.text())
            .map(|t| t.trim())
            .filter(|t| !t.is_empty())
            .collect::<Vec<_>>()
            .join(" ");

          if non_bq.is_empty() {
            (flat, false, None)
          } else {
            (non_bq, false, Some(bq))
          }
        }
      }
    } else {
      (String::new(), false, None)
    };

    if entry_text.is_empty() {
      activities.push(JournalActivity {
        event: "progress_updated".to_owned(),
        action_at,
        entry: String::new(),
        metadata: json!({ "progress_was": prev_progress, "progress": progress }),
      });
    } else {
      activities.push(JournalActivity {
        event: if all_blockquote { "quote" } else { "note" }.to_owned(),
        action_at,
        entry: entry_text,
        metadata: match quote_text {
          Some(q) => json!({ "quote": q }),
          None => Value::Null,
        },
      });
    }

    prev_progress = progress;
  }

  Ok(activities)
}

fn parse_storygraph_date(date_text: &str) -> String {
  let parts: Vec<&str> = date_text.trim().split_whitespace().collect();
  if parts.len() == 3 {
    let day: u32 = parts[0].parse().unwrap_or(1);
    let month: u32 = match parts[1].to_lowercase().as_str() {
      "january" => 1, "february" => 2, "march" => 3,    "april" => 4,
      "may" => 5,     "june" => 6,     "july" => 7,     "august" => 8,
      "september" => 9, "october" => 10, "november" => 11, "december" => 12,
      _ => 0,
    };
    let year: i32 = parts[2].parse().unwrap_or(2024);
    if month > 0 {
      if let Some(d) = chrono::NaiveDate::from_ymd_opt(year, month, day) {
        return format!("{}T12:00:00Z", d.format("%Y-%m-%d"));
      }
    }
  }
  date_text.to_owned()
}
