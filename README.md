# NickelStorygraph

A Kobo eReader plugin that integrates with [StoryGraph](https://app.thestorygraph.com). Syncs your reading progress automatically, logs highlights and notes as journal entries, and lets you manage your StoryGraph library directly from your device.

## Features

- **Progress sync** — automatically updates your StoryGraph reading progress as you read, configurable by threshold or on book close
- **Auto-link** — identifies the correct StoryGraph digital edition via ISBN or title/author search when you open a book for the first time
- **Book status** — set Want to Read, Currently Reading, Paused, Read, or Did Not Finish from the device; rereading is detected automatically when you re-open a finished book
- **Reading journal** — view your StoryGraph journal and add new entries with notes and privacy settings
- **Annotations** — sync Kobo highlights and notes to your StoryGraph journal
- **Ratings & reviews** — rate books (quarter-star), write reviews, tag moods, and answer book property questions; prompted automatically when you finish a book
- **Edition management** — manually link to any edition, switch editions, or sync your edition from changes made on the StoryGraph website

## Installation

1. Download `KoboRoot.tgz` from the [releases page](../../releases).
2. Connect your Kobo to your computer via USB.
3. Copy `KoboRoot.tgz` into the `.kobo/` folder on the device root.
4. Safely eject the Kobo — it will install the plugin automatically on reconnect.
5. Copy `config_example.ini` from `mnt/onboard/.adds/NickelStorygraph/` to the same folder, rename it to `config.ini`, and fill in your credentials (see below).

## Configuration

The config file lives at `.adds/NickelStorygraph/config.ini` on the Kobo's internal storage. Open `config_example.ini` for the full list of options with descriptions. The required settings are your StoryGraph session cookies:

```ini
; Obtain from browser DevTools → Application → Cookies → app.thestorygraph.com
session_cookie = <value of _storygraph_session>
remember_user_token = <value of remember_user_token>

; Optional — avoids a homepage request on every sync
user_slug = yourname
```

To get your cookies, log in to StoryGraph in a browser, open DevTools (`F12`), go to **Application → Storage → Cookies → `https://app.thestorygraph.com`**, and copy the values for `_storygraph_session` and `remember_user_token`.

### Key options

| Option | Default | Description |
|---|---|---|
| `auto_sync_default` | `false` | Whether auto-sync is enabled for new books by default |
| `sync_on_close` | `always` | When to auto-sync: `always`, `never`, or a percentage threshold (e.g. `5`) |
| `threshold` | `20` | Sync while reading if progress changes by more than this many percentage points |
| `sync_bookmarks` | `always` | When to sync highlights/notes: `always`, `never`, or `finished` |
| `sync_daily` | off | Hour of day to sync even if asleep (1–24, e.g. `2` for 2am) |

## Usage

Open any book and tap the StoryGraph icon in the toolbar to access the menu:

- **Sync now** — immediately push your current reading progress
- **Enable / Disable auto-sync** — toggle automatic syncing for this book
- **Book management**
  - **Manually link book** / **Unlink book** — search StoryGraph to link this book to a specific edition, or clear the link
  - **Sync edition from web** — re-checks your StoryGraph currently-reading shelf and updates the linked edition if you changed it on the website
- **Update book status** — set your reading status (Want to Read, Currently Reading, Paused, Read, Did Not Finish)
- **Open reading journal** — browse journal entries and add new ones
- **Write a review** — rate and review the book
- **Settings** — configure sync behaviour, credentials, and other options

### How auto-linking works

The first time a book is synced, NickelStorygraph looks up the correct StoryGraph edition automatically:

1. Extracts ISBNs from the EPUB metadata and searches StoryGraph
2. Falls back to a title + author search if no ISBN match is found
3. Prefers digital editions to avoid linking to the paperback or hardcover record
4. Saves the resolved edition ID so all subsequent syncs are instant

If a book can't be found automatically, a prompt appears to link it manually. Use **Book management → Sync edition from web** if you later change editions on StoryGraph's website.

## Building

The full build runs inside a Docker container using the [NickelTC](https://github.com/nickel-org/nickeltc) cross-compilation toolchain.

**Prerequisites:** Docker, [just](https://github.com/casey/just), [resvg](https://github.com/RazrFalcon/resvg)

```sh
# First time — build the toolchain image
just build-tc

# Build KoboRoot.tgz
just build-package

# Build and copy directly to a USB-connected Kobo (requires sudo)
just copy-package
```

Other useful commands:

| Command | Description |
|---|---|
| `just build-res` | Re-render SVG icons to PNG |
| `just build-hook` | Compile the Qt/C++ Nickel plugin only |
| `just build-cli` | Cross-compile the Rust CLI only |
| `just format` | Format all source files |
| `just clean` | Remove build artifacts |
| `just logs` | Stream logs from the Kobo over SSH (requires `KOBO_SERVER` and `KOBO_PASSWORD` env vars) |

## Architecture

NickelStorygraph has two components that communicate via subprocess calls and JSON:

- **Hook** (`hook/`) — a Qt5/C++ plugin that injects into Kobo's Nickel UI. Handles all UI rendering, menu items, dialogs, and user input. Spawns the CLI as a child process and reads its JSON output.
- **CLI** (`cli/`) — a Rust binary cross-compiled for ARM Linux. Contains all business logic: StoryGraph web scraping (session auth, progress updates, journal, reviews, search), Kobo SQLite queries for reading position and annotations, ISBN extraction from EPUB metadata, and edition resolution.

This separation means the CLI can be run and tested independently of the device UI.

## Acknowledgements

Built on top of [NickelHook](https://github.com/pgaskin/NickelHook) by Patrick Gaskin.
