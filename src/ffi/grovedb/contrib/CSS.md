# GroveDB Documentation Theme — CSS Notes

Detailed notes on the gcovr + Doxygen unified theme work.

## Architecture

Two separate documentation systems share one visual language:

| System | Location | Template Engine | Base CSS |
|--------|----------|-----------------|----------|
| **Doxygen** | `contrib/doxygen/` | Doxygen built-in | doxygen-awesome-css |
| **gcovr** | `contrib/gcovr/` | Jinja2 (ChoiceLoader) | GitHub Primer CSS |

Build script: `build_docs.py` at `src/ffi/grovedb/` root. Outputs to `./output/` (Doxygen), `./output/test/`, `./output/fuzz/`.

### File Map

**gcovr theme** (`contrib/gcovr/theme/`):
- `base.html` — Main Jinja2 template (sidebar, footer, dark mode JS)
- `style.css` — Master CSS (includes primer.css, style.common.css, style.colors.css via Jinja2)
- `style.colors.css` — Coverage color definitions (green success, red danger)
- `style.common.css` — Pygments light-mode syntax highlighting (upstream, unmodified)

**Doxygen theme** (`contrib/doxygen/`):
- `custom/header.html` — Custom Doxygen header (dark mode toggle placement JS)
- `custom/footer.html` — Custom Doxygen footer
- `custom/custom.css` — All custom overrides on top of doxygen-awesome
- `theme/doxygen-awesome*.css` — Upstream doxygen-awesome-css (unmodified)
- `theme/doxygen-awesome-darkmode-toggle.js` — Modified toggle (icons swapped to match gcovr)

### Shared Design Tokens

| Token | Value |
|-------|-------|
| Sidebar width | 335px (matches `--side-nav-fixed-width`) |
| Brand blue | `#0289DD` (light), `#182E3C` (dark) |
| Footer height | 35px (min-height) |
| Footer bg | `#ededed` (light), `#1c1c1c` (dark) |
| Footer font | 13px Inter |
| HR below logo/search | `1px solid rgba(255,255,255,0.25)` |
| Logo left padding | Doxygen: `padding-left: 10%`, gcovr: `padding-left: 5%` |
| Sidebar header bottom | gcovr: `padding: 28px 14px 88px` → total ~149px. Doxygen: `--top-height: 148px` |
| Toggle button | `border: 1px solid rgba(255,255,255,0.3); border-radius: 6px; padding: 6px 8px` |
| Dark mode key | `grovedb-dark-mode` in localStorage |

## Specificity Battles & Pitfalls

### 1. Primer CSS `!important` Everywhere

gcovr includes GitHub's Primer CSS via `{% include "primer.css" %}`. Primer uses `!important` on utility classes extensively. Every override must also use `!important` or use higher-specificity selectors.

**Lesson**: When overriding Primer, always use `!important`. There's no way around it.

### 2. Doxygen.css Hardcoded Values

`doxygen.css` hardcodes values that don't respect CSS variables:

```css
/* doxygen.css — can't be changed without !important */
#nav-path ul { height: 30px; line-height: 30px; }
font-size: var(--navigation-font-size)  /* resolves to 8pt */
```

Required aggressive overrides:
```css
#nav-path ul { height: auto !important; line-height: normal !important; font-size: 13px !important; }
```

**Lesson**: Always check what doxygen.css hardcodes before assuming CSS variables will work. Use browser devtools to trace which rule actually wins.

### 3. doxygen-awesome-sidebar-only.css Re-Overrides Variables

The sidebar-only theme re-declares CSS variables inside its own `@media screen and (min-width: 768px)` block, overriding root-level custom values. Had to re-assert searchbar colors inside the same media query:

```css
@media screen and (min-width: 768px) {
    html { --searchbar-background: rgba(255,255,255,0.15); }
}
```

**Lesson**: Order and specificity of `@media` blocks matters for CSS variable declarations.

### 4. Footer Positioning — Double Offset Trap

Doxygen's `#nav-path` sits outside `#container` in the DOM:
```html
<div id="container">
  <div id="doc-content">...</div>
</div>
<div id="nav-path">...</div>  <!-- sibling, not child -->
```

The sidebar-only theme applies `margin-left` to `#nav-path`. When we added `position: fixed; left: var(--side-nav-fixed-width)`, it double-offset the footer (margin-left + left). Fix: use `left: 0` and let the theme's existing margin-left handle alignment.

**Lesson**: Always check whether the theme already offsets an element before adding positional offsets. `position: fixed` removes the element from flow but `margin-left` from a class still applies if the selector matches.

### 5. `#doc-content` Height Calculation

doxygen-awesome-sidebar-only hardcodes:
```css
#doc-content { height: calc(100vh - 31px); }  /* assumes 30px+1px footer */
```

Our footer is 35px, so we override:
```css
#doc-content { height: calc(100vh - var(--footer-height)) !important; }
```

**Lesson**: When changing footer height, also update the content area height calculation.

### 6. Scroll Marker Background Bleed

gcovr's `#scroll_marker` element has a white background that bleeds through even when set to `background: transparent`. The `<html>` and `<body>` elements also lacked `background-color`, showing the browser default white behind.

Fix required both:
```css
#scroll_marker { display: none !important; }
html, body { background-color: var(--color-canvas-default); }
```

**Lesson**: `background: transparent` doesn't always visually fix things — the parent/root element's background shows through. Set `html, body` background explicitly.

### 7. Sidebar Scrollbar on Blue Background

The native scrollbar track was white on the blue sidebar. Required both standard and WebKit approaches:

```css
#sidebar-nav {
    scrollbar-color: rgba(255,255,255,0.3) transparent;
    scrollbar-width: thin;
}
#sidebar-nav::-webkit-scrollbar-track { background: transparent; }
#sidebar-nav::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.3); }
```

**Lesson**: Scrollbar styling needs both `scrollbar-color`/`scrollbar-width` (Firefox/standards) AND `::-webkit-scrollbar-*` pseudo-elements (Chrome/Safari).

### 8. Coverage Colors — Saturation for Readability

Initially used blue for coverage (matching Primer's `--color-accent-*`). Switched to green for semantic meaning. Then light mode overlays were too faint because the alpha was too low against black text.

Final light mode values:
```css
--color-success-muted: rgba(46, 160, 67, 0.35);   /* covered line bg */
--color-success-subtle: rgba(46, 160, 67, 0.18);   /* alt row bg */
--color-danger-subtle: rgba(207, 34, 46, 0.18);    /* uncovered line bg */
```

**Lesson**: Dark text on colored backgrounds needs higher alpha than white text does. When adjusting saturation, increase alpha (more opaque), don't decrease it.

### 9. Pygments Dark Mode — Dual Path Required

Pygments generates ~69 CSS classes (`.c`, `.c1`, `.k`, `.nf`, etc.) for syntax highlighting. gcovr includes these via `style.common.css` for light mode. Dark mode needs a complete parallel set.

Both `html.dark-mode .src .XX` selectors AND `@media (prefers-color-scheme: dark) { html:not(.light-mode) .src .XX }` blocks are needed to cover:
- Users who click the toggle (class-based)
- Users with system dark preference who never touch the toggle (media query)

**Lesson**: Always implement dark mode with both class toggle AND `prefers-color-scheme` media query with `:not(.light-mode)` guard. Either one alone leaves gaps.

### 10. Doxygen Dark Mode Toggle Placement

The `<doxygen-awesome-dark-mode-toggle>` web component's `.init()` places the toggle next to the search box. We needed it at the bottom of the sidebar (matching gcovr).

Solution: Skip `.init()`, manually create the element and append to a custom `#sidebar-toggle-footer` div in `header.html`:
```javascript
var toggle = document.createElement('doxygen-awesome-dark-mode-toggle');
toggle.title = DoxygenAwesomeDarkModeToggle.title;
toggle.updateIcon();
// ... event listeners ...
var footer = document.createElement('div');
footer.id = 'sidebar-toggle-footer';
footer.appendChild(toggle);
document.body.appendChild(footer);
```

**Lesson**: Doxygen-awesome web components can be manually instantiated. You don't have to use `.init()`. The static constructor in the JS file handles dark mode class application on `<html>` — the button is just UI.

### 11. Toggle Icon Convention

Swapped icon meaning to match gcovr convention:
- **Light mode shows**: Moon icon (click to switch to dark)
- **Dark mode shows**: Sun icon (click to switch to light)

Modified `doxygen-awesome-darkmode-toggle.js` — replaced colored SVGs (`#FCBF00`, `#FE9700`) with `currentColor` SVGs matching gcovr's simpler Material Icons style.

### 12. Hidden Doxygen UI Elements

Several Doxygen UI elements had to be forcibly hidden:
```css
.ui-resizable-handle, .ui-resizable-e, #nav-sync {
    display: none !important; width: 0 !important;
}
```

- `.ui-resizable-handle` / `.ui-resizable-e` — The `<->` resize handle between sidebar and content
- `#nav-sync` — "Click to disable panel synchronization" button

**Lesson**: These elements persist even when you think you've removed them. Use `display: none !important` with `width: 0 !important` to be thorough.

### 13. Mobile Responsive — gcovr Sidebar

The vertical sidebar becomes a horizontal nav bar on mobile (`max-width: 767px`):
- `flex-direction: row; flex-wrap: nowrap`
- Logo shrinks to 108x24px
- Files section hidden via `#nav-files-header` + sibling selectors + `:has()` selector
- Footer toggle loses its border, becomes compact
- Content `margin-left: 0`
- The HR (`border-bottom`) on `#sidebar-header` must be explicitly removed on mobile

**Lesson**: Every desktop-specific visual (HR, padding, fixed positioning) needs an explicit mobile override. Don't assume collapsing to horizontal removes decorative borders.

### 14. gcovr Output Directory Pre-Creation

gcovr fails if its output directory doesn't exist:
```
ValueError: fuzz.cfg: 7: output: Could not create output file 'output/fuzz/index.html': No such file or directory
```

The build script must `os.makedirs()` before running gcovr.

**Lesson**: gcovr doesn't create intermediate directories for its output path. Always pre-create them.

### 15. Doxyfile Version Injection

Doxygen's `PROJECT_NUMBER` field appears in every generated page. To inject the git hash:
1. Read original Doxyfile
2. Regex-replace `PROJECT_NUMBER = unknown` with the hash
3. Run doxygen
4. **Always** restore original in a `finally` block

**Lesson**: Use a `try/finally` to ensure the Doxyfile is never left with a baked-in hash. This prevents dirty git status.

### 16. Inverse Rounded Corner (Notch)

gcovr's footer has an inverse rounded corner where it meets the sidebar:
```css
#content-footer::before {
    background: radial-gradient(circle at 0% 100%, transparent 12px, #ededed 12px);
}
```

Attempted the same on Doxygen but it looked wrong because the sidebar and content area have different DOM structures. Removed from Doxygen.

**Lesson**: Decorative tricks that work in one layout may not transfer to another. The notch relies on the footer being a direct sibling of the sidebar edge — different DOM structures break the visual.

## Common Workflow

1. Edit CSS files in `contrib/doxygen/custom/` or `contrib/gcovr/theme/`
2. Run `python3 build_docs.py` from `src/ffi/grovedb/`
3. Open `output/index.html` (Doxygen) or `output/test/index.html` (gcovr) in browser
4. Compare side-by-side, check both light and dark mode
5. Test mobile by resizing browser to <768px width
