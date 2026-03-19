(function () {
  'use strict';

  var section = document.getElementById('comment-section');
  if (!section) return;

  var slug      = section.dataset.slug;
  var listEl    = document.getElementById('comment-list');
  var countEl   = document.getElementById('comment-count');
  var textarea  = document.getElementById('comment-body');
  var submitBtn = document.getElementById('comment-submit');

  /* ── Localised strings ─────────────────────────────────── */
  var i18n = {
    sectionTitle : '댓글',
    formTitle    : '댓글 작성',
    note         : 'GitHub 계정으로 댓글을 작성할 수 있습니다.',
    placeholder  : '댓글을 입력하세요...',
    submitLabel  : 'GitHub으로 댓글 달기',
    loading      : '로드 중...',
    empty        : '아직 댓글이 없습니다.',
    error        : '댓글을 불러올 수 없습니다.',
    alertEmpty   : '댓글을 입력해 주세요.',
    count        : function (n) { return n + '개의 댓글'; }
  };

  /* Populate static labels injected as empty elements by the generator */
  var el;
  el = document.getElementById('comment-section-title');
  if (el) el.textContent = i18n.sectionTitle;
  el = document.getElementById('comment-form-title');
  if (el) el.textContent = i18n.formTitle;
  el = document.getElementById('comment-form-note');
  if (el) el.textContent = i18n.note;
  if (textarea)  textarea.placeholder = i18n.placeholder;
  if (submitBtn) submitBtn.textContent = i18n.submitLabel;
  if (countEl)   countEl.textContent   = i18n.loading;

  /* ── HTML helpers ─────────────────────────────────────── */
  function appendTextSegment(container, str) {
    if (!container || str === undefined || str === null || str === '') return;
    var parts = String(str).split(/\r\n|\r|\n/);
    for (var i = 0; i < parts.length; i++) {
      container.appendChild(document.createTextNode(parts[i]));
      if (i !== parts.length - 1) container.appendChild(document.createElement('br'));
    }
  }

  function sanitizeUrl(input) {
    var url = String(input || '').trim();
    if (!url) return '';
    if (/^(?:https?:)?\/\//i.test(url)) return url;
    if (/^[a-z][a-z0-9+\-.]*:/i.test(url)) {
      return /^https?:/i.test(url) ? url : '';
    }
    if (url[0] === '/' || url[0] === '.') return url;
    if (url.indexOf(':') === -1) return url;
    return '';
  }

  var IMG_SYNTAX = /!\[([^\]]*)\]\(([^)]+)\)/g;

  function createImageFigure(url, alt) {
    var figure = document.createElement('figure');
    figure.className = 'comment-image';
    var img = document.createElement('img');
    img.src = url;
    img.alt = alt;
    figure.appendChild(img);
    if (alt) {
      var caption = document.createElement('figcaption');
      caption.textContent = alt;
      figure.appendChild(caption);
    }
    return figure;
  }

  function renderCommentBody(container, body) {
    if (!container) return;
    container.textContent = '';
    if (!body) return;
    var text = String(body);
    var lastIndex = 0;
    var match;
    IMG_SYNTAX.lastIndex = 0;
    while ((match = IMG_SYNTAX.exec(text)) !== null) {
      if (match.index > lastIndex) {
        appendTextSegment(container, text.slice(lastIndex, match.index));
      }
      var alt = match[1] || '';
      var url = sanitizeUrl(match[2]);
      if (url) {
        container.appendChild(createImageFigure(url, alt));
      } else {
        appendTextSegment(container, match[0]);
      }
      lastIndex = match.index + match[0].length;
    }
    if (lastIndex < text.length) {
      appendTextSegment(container, text.slice(lastIndex));
    }
  }

  /* ── Load & render comments ────────────────────────────── */
  fetch('/data/comments.json?v=' + Date.now(), { cache: 'no-store' })
    .then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    })
    .then(function (data) {
      var list = Array.isArray(data[slug]) ? data[slug] : [];
      if (countEl) {
        countEl.textContent = list.length ? i18n.count(list.length) : '';
      }
      if (!listEl) return;
      listEl.innerHTML = '';
      if (!list.length) {
        var emptyEl = document.createElement('p');
        emptyEl.className = 'no-comments';
        emptyEl.textContent = i18n.empty;
        listEl.appendChild(emptyEl);
        return;
      }
      var fragment = document.createDocumentFragment();
      list.forEach(function (c) {
        var commentEl = document.createElement('div');
        commentEl.className = 'comment';

        var metaEl = document.createElement('div');
        metaEl.className = 'comment-meta';
        var authorEl = document.createElement('span');
        authorEl.className = 'comment-author';
        authorEl.textContent = c.author || 'Anonymous';
        metaEl.appendChild(authorEl);
        var date = c.created_at ? c.created_at.slice(0, 10) : '';
        if (date) {
          var dateEl = document.createElement('span');
          dateEl.className = 'comment-date';
          dateEl.textContent = date;
          metaEl.appendChild(dateEl);
        }
        commentEl.appendChild(metaEl);

        var bodyEl = document.createElement('div');
        bodyEl.className = 'comment-body';
        renderCommentBody(bodyEl, c.body);
        commentEl.appendChild(bodyEl);

        fragment.appendChild(commentEl);
      });
      listEl.appendChild(fragment);
    })
    .catch(function () {
      if (countEl) countEl.textContent = '';
      if (listEl) {
        listEl.innerHTML = '';
        var errorEl = document.createElement('p');
        errorEl.className = 'no-comments';
        errorEl.textContent = i18n.error;
        listEl.appendChild(errorEl);
      }
    });

  /* ── Comment form ──────────────────────────────────────── */
  var REPO     = 'Religiya-Serdtsa/religiya-serdtsa.github.io';
  var BASE_URL = 'https://github.com/' + REPO + '/issues/new';

  if (submitBtn && textarea) {
    submitBtn.addEventListener('click', function (e) {
      e.preventDefault();
      var body = textarea.value.trim();
      if (!body) {
        alert(i18n.alertEmpty);
        return;
      }
      var issueTitle = '[comment] ' + slug;
      var issueUrl =
        BASE_URL +
        '?title=' + encodeURIComponent(issueTitle) +
        '&body='  + encodeURIComponent(body) +
        '&labels=comment';
      window.open(issueUrl, '_blank', 'noopener,noreferrer');
    });
  }
}());
