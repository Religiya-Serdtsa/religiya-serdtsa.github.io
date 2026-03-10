/**
 * search-ui.js — dynamic post search powered by the cwist WASM module.
 *
 * Expects on the page:
 *   #search-input       <input type="search">
 *   #search-results     container for result cards
 *   #search-no-result   element shown when query has no results (hidden attr)
 *
 * Also expects that search-module.js (Emscripten glue for cwist WASM)
 * has been loaded first, exposing CwistSearchModule() factory.
 *
 * Falls back to pure-JS scoring when the WASM module is unavailable.
 */
(function () {
  'use strict';

  var inputEl    = document.getElementById('search-input');
  var resultsEl  = document.getElementById('search-results');
  var noResultEl = document.getElementById('search-no-result');

  if (!inputEl || !resultsEl) return;

  var index   = null;   /* Array<{title,url,summary,tags,date}> */
  var wasmMod = null;   /* CwistSearchModule instance, or null  */
  var cursor  = -1;     /* keyboard-navigation index            */

  /* ── Initialise: load WASM, then the search index ────── */
  function init() {
    var wasmReady =
      (typeof CwistSearchModule === 'function')
        ? CwistSearchModule().catch(function () { return null; })
        : Promise.resolve(null);

    wasmReady
      .then(function (m) {
        wasmMod = m;
        return fetch('/search-index.json?v=' + Date.now(), { cache: 'no-store' });
      })
      .then(function (r) {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.json();
      })
      .then(function (data) {
        index = Array.isArray(data) ? data : [];
        inputEl.disabled = false;
        inputEl.focus();
      })
      .catch(function (e) {
        console.error('[cwist-search] init error:', e);
        inputEl.disabled = false;
      });
  }

  /* ── HTML helpers ─────────────────────────────────────── */
  function esc(s) {
    return String(s)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  function highlight(text, query) {
    if (!query || !text) return esc(text || '');
    var safeQ = query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    try {
      return esc(String(text)).replace(
        new RegExp(safeQ, 'gi'),
        function (m) { return '<mark>' + m + '</mark>'; }
      );
    } catch (_) { return esc(String(text)); }
  }

  /* ── Score a single post ──────────────────────────────── */
  function scorePost(post, query) {
    var tagsStr = (post.tags || []).join(' ');
    if (wasmMod) {
      return wasmMod.ccall(
        'cwist_score', 'number',
        ['string', 'string', 'string', 'string'],
        [query, post.title || '', tagsStr, post.summary || '']
      );
    }
    /* Pure-JS fallback (case-insensitive indexOf) */
    var q = query.toLowerCase();
    var s = 0;
    if ((post.title   || '').toLowerCase().indexOf(q) !== -1) s += 3;
    if (tagsStr.toLowerCase().indexOf(q)               !== -1) s += 2;
    if ((post.summary || '').toLowerCase().indexOf(q)  !== -1) s += 1;
    return s;
  }

  /* ── Render results for a query ───────────────────────── */
  function render(query) {
    var q = query.trim();
    if (!q || !index) {
      resultsEl.innerHTML = '';
      if (noResultEl) noResultEl.hidden = true;
      cursor = -1;
      return;
    }

    var scored = [];
    for (var i = 0; i < index.length; i++) {
      var s = scorePost(index[i], q);
      if (s > 0) scored.push({ post: index[i], score: s });
    }
    scored.sort(function (a, b) { return b.score - a.score; });

    if (!scored.length) {
      resultsEl.innerHTML = '';
      if (noResultEl) noResultEl.hidden = false;
      cursor = -1;
      return;
    }
    if (noResultEl) noResultEl.hidden = true;

    resultsEl.innerHTML = scored.slice(0, 10).map(function (x) {
      var p    = x.post;
      var tags = (p.tags || []).map(function (t) {
        return '<span class="search-tag">' + esc(t) + '</span>';
      }).join('');
      return (
        '<a class="search-result" href="' + esc(p.url || '/') + '">' +
          '<div class="sr-title">'   + highlight(p.title,   q) + '</div>' +
          (p.summary
            ? '<div class="sr-summary">' + highlight(p.summary, q) + '</div>'
            : '') +
          (tags ? '<div class="sr-tags">' + tags + '</div>' : '') +
        '</a>'
      );
    }).join('');

    cursor = -1;
  }

  /* ── Input handler ────────────────────────────────────── */
  inputEl.addEventListener('input', function () {
    render(inputEl.value);
  });

  /* ── Keyboard navigation (↑ ↓ Enter) ─────────────────── */
  inputEl.addEventListener('keydown', function (e) {
    var items = resultsEl.querySelectorAll('.search-result');
    if (!items.length) return;

    if (e.key === 'ArrowDown') {
      e.preventDefault();
      cursor = Math.min(cursor + 1, items.length - 1);
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      cursor = Math.max(cursor - 1, -1);
    } else if (e.key === 'Enter' && cursor >= 0) {
      e.preventDefault();
      items[cursor].click();
      return;
    } else {
      return;
    }

    items.forEach(function (el, i) {
      el.classList.toggle('focused', i === cursor);
      if (i === cursor) el.scrollIntoView({ block: 'nearest' });
    });
  });

  /* ── Boot ─────────────────────────────────────────────── */
  init();
}());
