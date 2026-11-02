# NickelStorygraph

## Announcement - Please Read!

Hi Fellow Redditors! It's come to my attention that this project has been shared on both Mobileread and Reddit over the last few weeks and while I'm very happy that others are getting use out of one of my personal pet projects I just want to set some expectations around support and updates. I built this project for myself because I often forget to update my progress on Storygraph and I knew there had to be a way to build an integration despite the lack of public API. 

At this point I didn't intend for this to be used by the masses, just a few friends who I convinced to come to the Kobo / StoryGraph dark side from Kindle. While all are absolutely welcome to use it in its current state, there may still be some issues and bugs that I haven't come across yet and support for resolution or feature requests etc. will be limited to what I can do in my free time.

> **Note:** This plugin does not currently support Firmware 5.x as Kobo devices are not yet on that version and those are all I have available for testing. I will upgrade the plugin to support later versions if / when Kobo makes the switch. 

Thanks for your understanding! 

Victoria 

<br>

## About 
A Kobo eReader plugin that integrates with [StoryGraph](https://app.thestorygraph.com). Syncs your reading progress automatically, logs highlights and notes as journal entries, and lets you view common StorygGraph features directly from your device.

> **Note:** Wi-Fi is required for all StoryGraph features. Because StoryGraph has no public API, this plugin works by scraping the website directly — it may break if StoryGraph makes changes to their UI.

- [Features](#features)
- [Screenshots](#screenshots)
- [Installation](#installation)
- [Uninstall](#uninstall)
- [Configuration](#configuration)
- [Usage](#usage)
- [Troubleshooting](#troubleshooting)
- [Building - Developer Info](#building---developer-info)
- [Architecture - Developer Info](#architecture---developer-info)
- [Acknowledgements](#acknowledgements)

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

### Home screen menu

| | | |
|---|---|---|
| ![Home screen tab bar menu](screenshots/home-menu.png) | ![Currently Reading](screenshots/currently-reading.png) | ![Currently Reading — Paused tab](screenshots/currently-reading-paused.png) |
| Tab bar menu | Currently Reading | Currently Reading — Paused |
| ![Community Feed](screenshots/community-feed.png) | ![Reading Goals and streak](screenshots/reading-goals.png) | |
| Community Feed | Reading Goals & streak | |

### In-book menu

| | |
|---|---|
| ![In-book reading toolbar menu](screenshots/in-book-menu.png) | ![Update book status](screenshots/book-status.png) |
| Reading toolbar menu | Update book status |

### Book linking

| | |
|---|---|
| ![Manually link book search results](screenshots/search-results.png) | ![Edition selection](screenshots/editions.png) |
| Manually link book | Edition selection |

### Journal & reviews

| | |
|---|---|
| ![Reading journal](screenshots/reading-journal.png) | ![Write a review — rating and thoughts](screenshots/review-ratings.png) |
| Reading journal | Write a review |
| ![Write a review — moods and book properties](screenshots/review-moods.png) | ![Write a review — Simple review mode](screenshots/review-simple.png) |
| Moods & book properties | Simple review mode |

### Settings

| | |
|---|---|
| ![Settings dialog](screenshots/settings.png) | ![Settings — book information and advanced](screenshots/settings-book-info.png) |
| Settings | Book information & advanced |

## Installation

1. Download `KoboRoot.tgz` from the [releases page](../../releases).
2. Connect your Kobo to your computer via USB.
3. Copy `KoboRoot.tgz` into the `.kobo/` folder on the device root. The `.kobo` folder may be hidden — on Windows, enable "Show hidden items" in File Explorer; on macOS, press `Cmd+Shift+.` in Finder.
4. Safely eject the Kobo — it will install the plugin automatically on reconnect.
5. Once the Kobo restarts, log in by following [Logging in to StoryGraph](#logging-in-to-storygraph) below. The plugin won't work until you do this.

## Uninstall

1. Connect your Kobo via USB.
2. Create an empty file named `NickelStorygraph_uninstall` in the root of the Kobo's internal storage (the same level as the `KOBOeReader` folder you see when you plug it in).
3. Safely eject the Kobo — the plugin will remove itself on reconnect and the file will be deleted automatically.

Alternatively, delete `.adds/NickelStorygraph/` from the device's internal storage manually and restart the Kobo.

## Configuration

All settings live in one file on the Kobo: `.adds/NickelStorygraph/config.ini`. `config_example.ini` in the same folder is only a reference copy. The plugin never reads it, so editing it does nothing.

### Logging in to StoryGraph

StoryGraph has no public API or login screen the plugin can use, so you log in by copying two cookies from your web browser into `config.ini`. You only need to do this once, and again whenever the cookies expire.

**Step 1: Get your cookies from a computer browser**

1. On a computer, log in to [app.thestorygraph.com](https://app.thestorygraph.com) in Chrome, Edge, or Firefox.
2. Press `F12` to open the developer tools.
3. Open the cookies list:
   - **Chrome / Edge:** **Application** tab → **Storage** → **Cookies** → `https://app.thestorygraph.com`
   - **Firefox:** **Storage** tab → **Cookies** → `https://app.thestorygraph.com`
4. Find these two rows and copy the text in their **Value** column (double-click the value to select it):
   - `_storygraph_session`
   - `remember_user_token`

   If you don't see `remember_user_token`, log out of StoryGraph, then log back in with **Remember me** ticked.

**Step 2: Find `config.ini` on your Kobo**

1. On the Kobo, open any book, tap the StoryGraph icon, and choose **Sync now**. The plugin creates `config.ini` the first time it runs. You'll see a "Please set session_cookie" error; that's expected for now.
2. Connect the Kobo to your computer via USB and open `.adds/NickelStorygraph/`. You need hidden files visible to see `.adds`, the same as for `.kobo` during installation.
3. Open `config.ini` in a plain text editor (Notepad, TextEdit, VS Code). Don't use Word.

> **Windows users:** File Explorer hides file extensions by default, so `config.ini` shows up as just **`config`**. That *is* the right file. Don't create a new file called `config.ini`: Windows will save it as `config.ini.txt` and the plugin will ignore it. To see real file names, turn on **View → Show → File name extensions** in File Explorer.

**Step 3: Fill in your login details**

At the top of `config.ini` you'll see these lines:

```ini
; session_cookie =
; remember_user_token =
; user_slug = yourname
```

The `;` at the start of a line makes the plugin ignore it. For each of the three lines mentioned above, **delete the `;` and the space after it** so the line starts with the setting name. Then paste your values after the `=`:

```ini
session_cookie = paste-your-_storygraph_session-value-here
remember_user_token = paste-your-remember_user_token-value-here
user_slug = yourname
```

- `user_slug` is optional. It's the part of your StoryGraph profile URL after `/profile/`. Setting it makes syncing slightly faster.
- Paste the values without quotes, and make sure each one stays on a single line.

**Step 4: Save and eject**

Save the file, safely eject the Kobo, and open a book. Tap the StoryGraph icon and choose **Sync now** to check it worked.

**Common mistakes**

| Symptom | Cause |
|---|---|
| "Please set session_cookie" after editing | The line still starts with `;`, or you edited `config_example.ini` or a `config.ini.txt` instead of `config.ini` |
| Two files named `config` and `config.ini` in the folder | The one shown as `config` is the real `config.ini`. Delete the one you created and edit the original |
| Worked before, now says the session has expired | Your cookies expired. Repeat Steps 1 and 3 with fresh values |

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
- **Update book status** — Set your reading status (Want to Read, Currently Reading, Paused, Read, Did Not Finish), and mark the book private or public with **Private: Yes / No**
- **Open reading journal** — Browse journal entries and add new ones
- **Write a review** — Rate and review the book
- **Settings** — Configure sync behaviour and other options (login cookies can only be set in `config.ini`)

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
The plugin may not have installed correctly. Reconnect the Kobo via USB and confirm that the `.adds/NickelStorygraph/` folder exists and contains the `cli` file. If it's missing, repeat the installation steps.

**I get "Please set session_cookie" even though I added my cookies.**
See [Common mistakes](#logging-in-to-storygraph) under Logging in. The usual causes are that the line still starts with `;`, or the values went into the wrong file (`config_example.ini`, or a `config.ini.txt` that Windows created). Reinstalling the plugin won't fix this.

**Syncing used to work but now I get an error about my session or credentials.**
Your session cookies have expired. Log back in to StoryGraph in a browser, copy fresh values for `_storygraph_session` and `remember_user_token`, and update `config.ini` ([Logging in](#logging-in-to-storygraph), Steps 1 and 3).

**A book can't be found or links to the wrong edition.**
Use **Book management → Manually link book** to search for the correct edition and link it. Once linked, all future syncs use that edition directly.

**Something else is broken.**
Go to **Settings → Save system logs**, then reconnect the Kobo via USB and retrieve the log file from `.adds/NickelStorygraph/` to share when reporting the issue.

## Building - Developer Info

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

## Architecture - Developer Info

NickelStorygraph has two components that communicate via subprocess calls and JSON:

- **Hook** (`hook/`) — A Qt5/C++ plugin that injects into Kobo's Nickel UI. Handles all UI rendering, menu items, dialogs, and user input. Spawns the CLI as a child process and reads its JSON output.
- **CLI** (`cli/`) — A Rust binary cross-compiled for ARM Linux. Contains all business logic: StoryGraph web scraping (session auth, progress updates, journal, reviews, search), Kobo SQLite queries for reading position and annotations, ISBN extraction from EPUB metadata, and edition resolution.


## Acknowledgements

- Forked from [NickelHardcover](https://codeberg.org/StrayRose/NickelHardcover) by Ava Johnson, a Kobo plugin for [Hardcover](https://hardcover.app). Large portions of the hook and CLI infrastructure are derived from that project.
- Uses [NickelHook](https://github.com/pgaskin/NickelHook) by Patrick Gaskin for Kobo plugin injection.
- Coexists with [NickelMenu](https://pgaskin.net/NickelMenu/) by Patrick Gaskin, whose source was read directly to make the two mods share the home screen tab bar correctly (chaining into NickelMenu's own button-injection hook so both buttons appear) and to size our tab bar icon at runtime the same way NickelMenu sizes its own custom menu icons.
- Built using [NickelTC](https://github.com/nickel-org/nickeltc) by Patrick Gaskin, the ARM cross-compilation toolchain for Kobo development.
