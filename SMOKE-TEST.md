# Smoke Test — NickelStorygraph Qt6 (firmware 5.18+)

Manual test checklist for verifying the Qt6 port on a real device. Work through
every item in order; for any failure, capture the `logread` output (section 8)
before continuing.

## 0. Prerequisites

- [ ] A Kobo/Tolino on firmware **5.18.x** (or 6.0).
- [ ] Built artifacts: `hook/Kobo.tgz` and `mnt/onboard/.adds/NickelStorygraph/`
      (from `just build-package`).
- [ ] A backup of `.kobo/` and `.adds/` (or a recent full-device backup).
- [ ] StoryGraph session cookies ready for `config.ini`.

## 1. Install

- [ ] Connect the device via USB.
- [ ] Copy `hook/Kobo.tgz` into the device root's `.kobo/` folder.
- [ ] Copy `mnt/onboard/.adds/NickelStorygraph/` into `.adds/`, so that you get
      `.adds/NickelStorygraph/`.
- [ ] Safely eject — the device installs the plugin and restarts.

## 2. First boot — plugin loads (NickelHook Failsafe)

- [ ] After the restart, reconnect via USB.
- [ ] `.adds/NickelStorygraph/` still exists and contains `cli` and
      `config_example.ini` (it was not deleted).
- [ ] No `NickelStorygraph_uninstall` file appeared (nothing self-uninstalled).
- [ ] Nickel still boots normally (no brick, no boot loop).
- [ ] `logread | grep -i storygraph` shows plugin `nh_log` lines and no
      `nh_failsafe` / crash lines.

## 3. Configuration

- [ ] Copy `config_example.ini` to `config.ini` (same folder).
- [ ] Fill in `session_cookie` and `remember_user_token` (browser DevTools →
      Application → Cookies → app.thestorygraph.com).
- [ ] (Optional) set `user_slug`.
- [ ] Safely eject; confirm the config parses (no config error in the log).

## 4. Critical Qt6 changes — verify explicitly

### 4a. `Content::getId(QString())` — book identification

The Qt6 ABI change added a `QString const&` argument to `Content::getId`; the
hook passes an empty `QString()`. Verify the book is still identified correctly:

- [ ] Open a book that is already linked to a StoryGraph edition.
- [ ] Check **Settings → Book information**: the detected title/author/ID look
      correct (not empty or garbage).
- [ ] Reading progress syncs to `app.thestorygraph.com` (check the website).
- [ ] If instead you get "book not found" / a mismatched edition, the empty
      `QString` is not the right argument — capture `logread` and report.

### 4b. `ReadingMenuView(QWidget*, bool, bool)` — reading-menu injection

The Qt6 ABI change merged the two ReadingMenuView constructors into one
`(QWidget*, bool, bool)`. Verify the menu still appears:

- [ ] Open a book (the reading screen).
- [ ] Open the reading menu — a **StoryGraph** entry is present.
- [ ] The entry opens the StoryGraph actions without crashing.

## 5. Functional checks

### Home screen
- [ ] The StoryGraph tab-bar button appears (next to NickelMenu if installed).
- [ ] Tapping it opens the StoryGraph home panel.

### Sync
- [ ] Reading progress updates on StoryGraph on book close / threshold.
- [ ] A highlight or note syncs as a journal entry (if `sync_bookmarks` on).
- [ ] Manual sync from the menu works.

### Other UI
- [ ] Journal, reviews, search, editions and settings dialogs all open without
      crashing.

## 6. NickelMenu coexistence

- [ ] With NickelMenu installed, both buttons render in the home tab bar.
- [ ] Neither menu is missing; no duplicate/overlapping icon.

## 7. Uninstall

- [ ] Create an empty file `NickelStorygraph_uninstall` in the device root.
- [ ] Safely eject — the plugin removes itself and the file is deleted.
- [ ] Alternatively delete `.adds/NickelStorygraph/` and restart.

## 8. Collecting logs for a bug report

```sh
# over SSH (devmode) or telnet on the device:
logread | grep -iE 'storygraph|nh\.' > /tmp/storygraph.log
# copy /tmp/storygraph.log off the device
```

Include the firmware version and the exact step that failed together with
`/tmp/storygraph.log`.
