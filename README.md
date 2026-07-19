# NickelStorygraph

A Kobo eReader plugin that integrates with [StoryGraph](https://app.thestorygraph.com). Syncs your reading progress automatically, logs highlights and notes as journal entries, and lets you manage your StoryGraph library directly from your device.

> **Note:** Wi-Fi is required for all StoryGraph features. Because StoryGraph has no public API, this plugin works by scraping the website directly — it may break if StoryGraph makes changes to their UI.

## Features

### Syncing & book linking

- **Progress sync** — Automatically updates your StoryGraph reading progress as you read, configurable by threshold or on book close
- **Auto-link** — Identifies the correct StoryGraph digital edition via ISBN or title/author search when you open a book for the first time
- **Edition management** — Manually link to any edition, switch editions, or sync your edition from changes made on the StoryGraph website
- **Book status** — Set Want to Read, Currently Reading, Paused, Read, or Did Not Finish from the device; rereading is detected automatically when you re-open a finished book

### Journal & reviews

- **Reading journal** — View your StoryGraph journal and add new entries with notes
- **Annotations** — Sync Kobo highlights and notes to your StoryGraph journal, each highlight as its own journal entry
- **Ratings & reviews** — Rate books (quarter-star), write reviews, tag moods, and answer book property questions; prompted automatically when you finish a book on your Kobo. Enable **Simple review** in Settings to show only the rating and thoughts fields

### Home screen menu

Accessible from the Kobo home screen without opening a book (works alongside [NickelMenu](https://pgaskin.net/NickelMenu/) if it's also installed):

- **Currently Reading** — Current and paused books with progress, in a tabbed view
- **Community Feed** — Scrollable view of your friends' reading progress
- **Reading Goals** — Progress towards your Page / Minutes / Books goals set on StoryGraph, plus your reading streak (current streak, longest streak, and streak requirement) if you have one set up

## Screenshots

<!-- Add screenshots here -->

## Installation

1. Download `KoboRoot.tgz` from the [releases page](../../releases).
2. Connect your Kobo to your computer via USB.
3. Copy `KoboRoot.tgz` into the `.kobo/` folder on the device root. The `.kobo` folder may be hidden — on Windows, enable "Show hidden items" in File Explorer; on macOS, press `Cmd+Shift+.` in Finder.
4. Safely eject the Kobo — it will install the plugin automatically on reconnect.
5. Once the Kobo restarts, reconnect via USB. Open the `.adds/NickelStorygraph/` folder on the device, copy `config_example.ini`, rename the copy to `config.ini`, and fill in your credentials (see below).

## Uninstall

1. Connect your Kobo via USB.
2. Create an empty file named `NickelStorygraph_uninstall` in the root of the Kobo's internal storage (the same level as the `KOBOeReader` folder you see when you plug it in).
3. Safely eject the Kobo — the plugin will remove itself on reconnect and the file will be deleted automatically.

Alternatively, delete `.adds/NickelStorygraph/` from the device's internal storage manually and restart the Kobo.

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

Session cookies expire periodically. If syncing stops working, update these values in `config.ini` with fresh cookies from your browser.

### Key options

| Option | Default | Description |
|---|---|---|
| `auto_sync_default` | `false` | Whether auto-sync is enabled for new books by default |
| `sync_on_close` | `always` | When to auto-sync: `always`, `never`, or a percentage threshold (e.g. `5`) |
| `threshold` | `20` | Sync while reading if progress changes by more than this many percentage points |
| `sync_bookmarks` | `always` | When to sync highlights/notes: `always`, `manual` (only on manual sync), `finished` (only on book completion), or `never` |
| `sync_daily` | off | Hour of day to sync even if asleep (1–24, e.g. `2` for 2am) |
| `home_menu` | `true` | Show the StoryGraph home screen menu (see below) |
| `home_menu_reading` / `home_menu_feed` / `home_menu_goals` | `true` | Show/hide individual items in the home screen menu |

## Usage

Open any book and tap the StoryGraph icon in the reading toolbar at the top of the screen to access the menu. **Auto-sync is disabled per book by default** — use **Enable auto-sync** from the menu the first time you open a book, or set `auto_sync_default = true` in `config.ini` to enable it automatically for all books.

- **Sync now** — Immediately push your current reading progress
- **Enable / Disable auto-sync** — Toggle automatic syncing for this book
- **Book management**
  - **Manually link book** / **Unlink book** — Search StoryGraph to link this book to a specific edition, or clear the link
  - **Sync edition from web** — Re-checks your StoryGraph currently-reading shelf and updates the linked edition if you changed it on the website
- **Update book status** — Set your reading status (Want to Read, Currently Reading, Paused, Read, Did Not Finish)
- **Open reading journal** — Browse journal entries and add new ones
- **Write a review** — Rate and review the book
- **Settings** — Configure sync behaviour, credentials, and other options

### Home screen menu

A **StoryGraph** button in the home screen's tab bar (alongside Home/Discover/My Books/etc.) opens a menu with Currently Reading, Community Feed, Reading Goals, and Settings, without needing to open a book first. This works the same way whether or not [NickelMenu](https://pgaskin.net/NickelMenu/) is also installed — both mods add their own tab bar button side by side.

Disable this entirely with `home_menu = false` in `config.ini`, or hide individual items with `home_menu_reading`, `home_menu_feed`, `home_menu_goals`.

Your current reading streak (current streak, longest streak, and your streak requirement) shows at the bottom of the **Reading Goals** panel if you have one set up on StoryGraph.

### How auto-linking works

The first time a book is synced, NickelStorygraph looks up the correct StoryGraph edition automatically:

1. Extracts ISBNs from the EPUB metadata and searches StoryGraph
2. Falls back to a title + author search if no ISBN match is found
3. Prefers digital editions to avoid linking to the paperback or hardcover record
4. Saves the resolved edition ID so all subsequent syncs are instant

If a book can't be found automatically, a prompt appears to link it manually. Use **Book management → Sync edition from web** if you later change editions on StoryGraph's website.

## Troubleshooting

**The StoryGraph icon doesn't appear when I open a book.**
The plugin may not have installed correctly. Reconnect the Kobo via USB and confirm that `.adds/NickelStorygraph/nickelstoregraph.so` exists on the device. If the file is missing, repeat the installation steps.

**Syncing does nothing / I get an error about credentials.**
Your session cookies have expired. Log back in to StoryGraph in a browser, copy fresh values for `_storygraph_session` and `remember_user_token`, and update `config.ini`.

**A book can't be found or links to the wrong edition.**
Use **Book management → Manually link book** to search for the correct edition and link it. Once linked, all future syncs use that edition directly.

**Something else is broken.**
Go to **Settings → Save system logs**, then reconnect the Kobo via USB and retrieve the log file from `.adds/NickelStorygraph/` to share when reporting the issue.

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

- **Hook** (`hook/`) — A Qt5/C++ plugin that injects into Kobo's Nickel UI. Handles all UI rendering, menu items, dialogs, and user input. Spawns the CLI as a child process and reads its JSON output.
- **CLI** (`cli/`) — A Rust binary cross-compiled for ARM Linux. Contains all business logic: StoryGraph web scraping (session auth, progress updates, journal, reviews, search), Kobo SQLite queries for reading position and annotations, ISBN extraction from EPUB metadata, and edition resolution.

## Acknowledgements

- Forked from [NickelHardcover](https://codeberg.org/StrayRose/NickelHardcover) by Ava Johnson, a Kobo plugin for [Hardcover](https://hardcover.app). Large portions of the hook and CLI infrastructure are derived from that project.
- Uses [NickelHook](https://github.com/pgaskin/NickelHook) by Patrick Gaskin for Kobo plugin injection.
- Coexists with [NickelMenu](https://pgaskin.net/NickelMenu/) by Patrick Gaskin, whose source was read directly to make the two mods share the home screen tab bar correctly (chaining into NickelMenu's own button-injection hook so both buttons appear) and to size our tab bar icon at runtime the same way NickelMenu sizes its own custom menu icons.
- Built using [NickelTC](https://github.com/nickel-org/nickeltc) by Patrick Gaskin, the ARM cross-compilation toolchain for Kobo development.
