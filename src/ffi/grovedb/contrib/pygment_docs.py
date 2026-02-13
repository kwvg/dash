#!/usr/bin/env python3
"""Re-highlight Doxygen HTML fragments using Pygments CppLexer.

Replaces Doxygen's limited 7-class syntax highlighting with Pygments' full
C++ tokenization, adding support for numeric literals, better preprocessor
handling, and more accurate keyword classification.

Cross-reference links (``<a class="code hl_*">``) and structural elements
(line anchors, line numbers, fold markers) are preserved intact.
"""

import glob
import html
import os
import re

from pygments import lex
from pygments.lexers import CppLexer
from pygments.token import Token

# ---------------------------------------------------------------------------
# Token -> CSS class mapping
# ---------------------------------------------------------------------------

_TOKEN_CSS = {
    # Keywords
    Token.Keyword:             "keyword",
    Token.Keyword.Type:        "keywordtype",
    # Strings (including char literals — Pygments uses String.Char)
    Token.Literal.String:      "stringliteral",
    # Numbers — the raison d'etre of this module
    Token.Literal.Number:      "number",
    # Comments
    Token.Comment:             "comment",
    Token.Comment.Preproc:     "preprocessor",
    Token.Comment.PreprocFile: "stringliteral",
    # Function names (identifiers followed by '(')
    Token.Name.Function:       "name-function",
}

# Tokens that should never receive a <span>, even if an ancestor matches.
_PLAIN = frozenset({
    Token.Text, Token.Text.Whitespace,
    Token.Punctuation, Token.Operator,
    Token.Name, Token.Name.Other,
})


def _css_class(tok):
    """Return the CSS class for a Pygments token, or *None* for plain text."""
    if tok in _PLAIN:
        return None
    t = tok
    while t is not Token:
        if t in _TOKEN_CSS:
            return _TOKEN_CSS[t]
        t = t.parent
    return None


# ---------------------------------------------------------------------------
# HTML parsing helpers
# ---------------------------------------------------------------------------

# Prefix: line anchor + lineno at the start of each <div class="line">
_PREFIX_RE = re.compile(
    r'^'
    r'(?P<anchor><a\s+(?:id|name)="l\d+"[^>]*></a>)?'
    r'\s*'
    r'(?P<lineno><span\s+class="lineno">.*?</span>)?',
    re.DOTALL,
)

# Cross-reference link produced by Doxygen
_LINK_RE = re.compile(
    r'<a\s+class="code\s+hl_\w+"[^>]*>.*?</a>',
    re.DOTALL,
)

# Named anchor (zero-width, for example targets).  Exclude line anchors
# whose id is "l" + digits.
_ANCHOR_RE = re.compile(
    r'<a\s+(?:'
    r'id="(?!l\d+")[^"]*"\s+name="[^"]*"'
    r'|'
    r'name="[^"]*"\s+id="(?!l\d+")[^"]*"'
    r')[^>]*></a>',
)

_TAG_RE = re.compile(r'<[^>]+>')
_ENTITY_RE = re.compile(r'&[#\w]+;')


def _split_prefix(line_html):
    """Return ``(prefix_html, code_html)`` from a ``<div class="line">`` body."""
    m = _PREFIX_RE.match(line_html)
    end = m.end() if m else 0
    return line_html[:end], line_html[end:]


def _parse_code(code_html):
    """Extract source text, cross-ref links, and named anchors.

    Returns ``(text, links, anchors)`` where:

    * *text* — decoded plain-text source
    * *links* — ``[(char_start, char_end, original_html), ...]``
    * *anchors* — ``[(char_offset, original_html), ...]``
    """
    links, anchors, parts = [], [], []
    pos, tlen = 0, 0

    while pos < len(code_html):
        # Cross-reference link
        m = _LINK_RE.match(code_html, pos)
        if m:
            raw = m.group(0)
            visible = html.unescape(_TAG_RE.sub("", raw))
            s = tlen
            tlen += len(visible)
            links.append((s, tlen, raw))
            parts.append(visible)
            pos = m.end()
            continue

        # Named anchor (zero-width)
        m = _ANCHOR_RE.match(code_html, pos)
        if m:
            anchors.append((tlen, m.group(0)))
            pos = m.end()
            continue

        # Any other tag — skip
        m = _TAG_RE.match(code_html, pos)
        if m:
            pos = m.end()
            continue

        # HTML entity
        m = _ENTITY_RE.match(code_html, pos)
        if m:
            decoded = html.unescape(m.group(0))
            parts.append(decoded)
            tlen += len(decoded)
            pos = m.end()
            continue

        # Literal character
        parts.append(code_html[pos])
        tlen += 1
        pos += 1

    return "".join(parts), links, anchors


# ---------------------------------------------------------------------------
# Reconstruction
# ---------------------------------------------------------------------------

def _wrap(text, cls):
    """Wrap *text* in ``<span class="cls">`` or return escaped plain text."""
    e = html.escape(text)
    return f'<span class="{cls}">{e}</span>' if cls else e


def _rebuild(source, tokens, infos, offsets):
    """Rebuild per-line HTML from Pygments tokens, preserving cross-ref links.

    Parameters
    ----------
    source  : str            — full source text (lines joined by ``\\n``)
    tokens  : list           — ``[(token_type, text), ...]`` from Pygments
    infos   : list           — ``[(prefix, text, links, anchors), ...]``
    offsets : list[int]      — global char offset for each source line
    """
    # Flatten links and anchors to global coordinates
    link_at = {}     # gpos -> (end_gpos, html)
    in_link = set()  # all gpos values covered by a link
    anchor_at = {}   # gpos -> [html, ...]

    for i, (_, _, links, anchors) in enumerate(infos):
        base = offsets[i]
        for s, e, h in links:
            gs, ge = base + s, base + e
            link_at[gs] = (ge, h)
            in_link.update(range(gs, ge))
        for o, h in anchors:
            anchor_at.setdefault(base + o, []).append(h)

    # Walk tokens and build output lines
    gpos = 0
    li = 0
    buf = ""
    buf_cls = None
    line_html = infos[0][0] if infos else ""
    result = []

    def flush():
        nonlocal buf, buf_cls, line_html
        if buf:
            line_html += _wrap(buf, buf_cls)
            buf = ""
            buf_cls = None

    for tok_type, tok_text in tokens:
        tok_cls = _css_class(tok_type)

        for ch in tok_text:
            if ch == "\n":
                flush()
                result.append(line_html)
                li += 1
                line_html = infos[li][0] if li < len(infos) else ""
                gpos += 1
                continue

            # Named anchors (zero-width)
            if gpos in anchor_at:
                flush()
                for ah in anchor_at[gpos]:
                    line_html += ah

            # Cross-ref link starts here -> emit entire <a> tag
            if gpos in link_at:
                flush()
                _, lh = link_at[gpos]
                line_html += lh

            # Inside a link -> skip (the <a> was already emitted)
            if gpos in in_link:
                gpos += 1
                continue

            # Regular character — group consecutive same-class chars
            if tok_cls != buf_cls:
                flush()
                buf_cls = tok_cls
            buf += ch
            gpos += 1

    flush()
    result.append(line_html)
    return result


# ---------------------------------------------------------------------------
# Fragment processing
# ---------------------------------------------------------------------------

# Matches the *content* (group 1) inside each <div class="line">.
_LINE_RE = re.compile(
    r'<div class="line">(.*?)(?=<div class="(?:line|fold)|</div>)',
    re.DOTALL,
)


def _process_fragment(line_matches, lexer):
    """Re-highlight one fragment's ``<div class="line">`` contents.

    Returns a list of new content strings, one per line match.
    """
    # Parse each line
    infos = []
    for m in line_matches:
        prefix, code_html = _split_prefix(m.group(1))
        text, links, anchors = _parse_code(code_html)
        infos.append((prefix, text, links, anchors))

    # If there's no actual source to highlight, return originals unchanged
    source = "\n".join(info[1] for info in infos)
    if not source.strip():
        return [m.group(1) for m in line_matches]

    # Tokenize
    tokens = list(lex(source, lexer))

    # Drop the trailing newline token that Pygments may add (ensurenl)
    if tokens and tokens[-1] == (Token.Text.Whitespace, "\n"):
        tokens.pop()

    # Compute line offsets
    offsets, off = [], 0
    for info in infos:
        offsets.append(off)
        off += len(info[1]) + 1  # +1 for the joining \n

    return _rebuild(source, tokens, infos, offsets)


def _process_file(content, lexer):
    """Process every fragment block in a single HTML file."""
    frag_starts = [m.start() for m in re.finditer(r'<div class="fragment">', content)]
    if not frag_starts:
        return content

    all_lines = list(_LINE_RE.finditer(content))
    if not all_lines:
        return content

    # Group lines by their enclosing fragment (largest frag_start <= line pos)
    frag_groups = {i: [] for i in range(len(frag_starts))}
    for lm in all_lines:
        ls = lm.start()
        fi = -1
        for i, fs in enumerate(frag_starts):
            if fs <= ls:
                fi = i
            else:
                break
        if fi >= 0:
            frag_groups[fi].append(lm)

    # Re-highlight each group (only fragments with line numbers — i.e.
    # source listings and examples.  Inline @code blocks and wire-format
    # descriptions lack <span class="lineno"> and must be left alone.)
    replacements = []
    for fi in sorted(frag_groups):
        lines = frag_groups[fi]
        if not lines:
            continue
        if not any('class="lineno"' in m.group(1) for m in lines):
            continue
        new_contents = _process_fragment(lines, lexer)
        for lm, nc in zip(lines, new_contents):
            replacements.append((lm.start(1), lm.end(1), nc))

    # Apply in reverse order to preserve positions
    replacements.sort(key=lambda r: r[0], reverse=True)
    for s, e, nc in replacements:
        content = content[:s] + nc + content[e:]

    return content


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def rehighlight(output_dir):
    """Re-highlight all Doxygen HTML files in *output_dir*.

    Returns the number of files modified.
    """
    lexer = CppLexer()
    count = 0

    for path in sorted(glob.glob(os.path.join(output_dir, "*.html"))):
        with open(path) as f:
            content = f.read()
        if '<div class="fragment">' not in content:
            continue

        new_content = _process_file(content, lexer)
        if new_content != content:
            with open(path, "w") as f:
                f.write(new_content)
            count += 1

    return count
