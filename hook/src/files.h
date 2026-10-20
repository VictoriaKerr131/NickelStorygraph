#pragma once

namespace Files {
constexpr char const *config = "/mnt/onboard/.adds/NickelStorygraph/config.ini";
constexpr char const *settings = "/mnt/onboard/.adds/NickelStorygraph/settings.ini";
constexpr char const *koboSettings = "/mnt/onboard/.kobo/Kobo/Kobo eReader.conf";
constexpr char const *cli = "/mnt/onboard/.adds/NickelStorygraph/cli";
constexpr char const *adds_directory = "/mnt/onboard/.adds/NickelStorygraph";

// Cleanup target for hardcover_uninstall(): earlier builds dropped a
// "StoryGraph" entry into NickelMenu's own config directory; no longer
// written, but still removed on uninstall in case it's present.
constexpr char const *nickelmenu_config_entry = "/mnt/onboard/.adds/nm/storygraph";

constexpr char const *arrow_backward = ":/images/reading/global_backward.png";
constexpr char const *arrow_down = ":/images/widgets/settings_date_time_down.png";
constexpr char const *arrow_forward = ":/images/reading/global_forward.png";
constexpr char const *arrow_left = ":/nickelstorygraph/arrow_left.png";
constexpr char const *arrow_menu = ":/images/menu/settings_menu_arrow.png";
constexpr char const *arrow_right = ":/images/menu/label_arrow_right.png";
constexpr char const *arrow_up = ":/images/widgets/settings_date_time_up.png";
constexpr char const *clear = ":/nickelstorygraph/clear.png";
constexpr char const *icon = ":/nickelstorygraph/icon.png";
constexpr char const *icon_hit = ":/nickelstorygraph/icon_hit.png";
constexpr char const *icon_hit_dark = ":/nickelstorygraph/icon_hit_dark.png";
constexpr char const *icon_outline = ":/nickelstorygraph/icon_outline.png"; // currently unused, kept for future use
// High-res source for the home screen tab bar icon, scaled at runtime to
// match Nickel's own native tab icon size - see scaledNavIconPath() in
// nickelstorygraph.cc. nav_icon/nav_icon_hit below are the static fallback
// used only if that runtime scaling fails.
constexpr char const *icon_master_outline = ":/nickelstorygraph/icon-master-outline.png";
constexpr char const *nav_icon = ":/nickelstorygraph/nav_icon.png";
constexpr char const *nav_icon_hit = ":/nickelstorygraph/nav_icon_hit.png";
constexpr char const *left_star = ":/nickelstorygraph/left_star.png";
constexpr char const *left_star_hit = ":/nickelstorygraph/left_star_hit.png";
constexpr char const *loading = ":/nickelstorygraph/loading.png";
constexpr char const *right_star = ":/nickelstorygraph/right_star.png";
constexpr char const *right_star_hit = ":/nickelstorygraph/right_star_hit.png";
constexpr char const *wifi = ":/images/statusbar/wifi05.png";

constexpr char const *list_book = ":/nickelstorygraph/list_book.png";
constexpr char const *note = ":/nickelstorygraph/note.png";
constexpr char const *private_book = ":/nickelstorygraph/private.png";
constexpr char const *progress_updated =":/nickelstorygraph/progress_updated.png";
constexpr char const *prompt_book = ":/nickelstorygraph/prompt_book.png";
constexpr char const *quote = ":/nickelstorygraph/quote.png";
constexpr char const *rated = ":/nickelstorygraph/rated.png";
constexpr char const *reviewed = ":/nickelstorygraph/reviewed.png";
constexpr char const *status_read = ":/nickelstorygraph/status_read.png";
constexpr char const *status_stopped = ":/nickelstorygraph/status_stopped.png";
constexpr char const *status_want_to_read = ":/nickelstorygraph/status_want_to_read.png";
constexpr char const *user_book_read_finished = ":/nickelstorygraph/user_book_read_finished.png";
constexpr char const *user_book_read_started = ":/nickelstorygraph/user_book_read_started.png";
} // namespace Files
