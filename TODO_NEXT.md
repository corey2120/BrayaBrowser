# Braya Browser – Next Session TODO

## Recent Progress
- ✅ Restored Settings → Privacy → “Manage Site Permissions” to list stored-site entries again and wired the per-site “Clear” controls to the WebKit website data manager (the `G_CALLBACK` handler now removes the selected cookies before trimming the list).
- ✅ Made the multi-step login capture toggle reuse the `createSettingsRow` layout so it matches the other switches and no longer dominates the card.

## Password Manager & Autofill
1. **Autofill feedback polish**
   - Add “Undo” or “View details” link in the toast so users can jump to the vault entry.
   - Expose toast timeout in settings (short/medium/long) or dismiss-on-click.

2. **Per-site exceptions manager**
   - UI list of domains in `passwords.ignore` with ability to re-enable saving.
   - Surface these under Settings → Privacy/Passwords.

3. **Autofill dropdown enhancements**
   - Anchor dropdown to focused field using translate coordinates per frame (account for inner iframes).
   - Add keyboard navigation (Up/Down/Enter/Escape) for selecting entries.

4. **Save/update dialog refinements**
   - Inline warning when passwords differ from stored entry; show last updated timestamp.
   - Persist “Never for this site” decision with confirmation snackbar.

5. **Multi-step capture UX**
   - Provide tooltip/help text near the toggle explaining when to disable it.
   - Offer “Always allow for this site” override when we detect a two-step login.

## Settings & Privacy
1. **Cookie/site permissions**
   - Build Privacy → “Cookies & Site Data” manager with per-site delete.
   - Wire “Manage site permissions” button to an actual dialog/panel.

2. **Theme sidebar rework**
   - Continue Vivaldi-style settings redesign (nav rail + cards).
   - Add consistent slidebars/toggles across General/Security tabs.

## Technical Follow-ups
1. **GTK deprecations**
   - Replace GtkFileChooserNative usage with GtkFileDialog once GTK 4.10 APIs are available.
2. **Password detection heuristics**
   - Detect alias fields (e.g., `data-test="username"`) and cross-frame logins.
3. **Tests & logging**
   - Add unit tests for password storage/migration and autofill logic.

## Nice-to-haves
- Toast/notification stack for other events (downloads, extensions).
- Optional “auto-copy username/password” buttons in the detail pane.
- Hotkey to open autofill dropdown (e.g., Ctrl+Shift+L).
