---
title: Welcome to the cwist Material UI Blog
date: 2024-05-25
excerpt: GitHub Pages 전용으로 설계된 cwist + Markdown 파이프라인과 프론트엔드를 소개합니다.
tags: Welcome, Guide, Overview
reading_minutes: 4
---

# Welcome to the cwist Material UI Blog

> **Purpose** — GitHub Pages에 올릴 수 있는 순수 정적 블로그 파이프라인(카테고리 INI + Markdown 프런트 매터 + md4c 렌더러)을 한눈에 소개합니다.

## 1. 플랫폼 한눈에 보기

| Layer | 구성 요소 | 설명 |
|-------|-----------|------|
| 카테고리 정의 | `categories.cfg` | INI 형식으로 제목, 설명, accent 색상, 순서를 선언 |
| 데이터 | `posts/<카테고리>/슬러그.md` | YAML 프런트 매터(`title`, `date`, `tags`, `reading_minutes`)와 Markdown 본문 |
| 렌더링 | `bin/bloggen` | md4c + C 기반 제너레이터가 HTML을 생성 (서버 필요 없음) |
| 스타일 | `assets/styles.css` | Cards/nav/타이포 등 공통 테마, 생성 시 `docs/assets/styles.css`로 복사 |
| 배포 | GitHub Pages Actions | 루트 소스로 `docs/` 산출물을 생성해 Pages에 업로드 |

## 2. 빠른 시작 순서

1. `categories.cfg`에 카테고리 메타데이터를 추가하거나 수정합니다.
2. `posts/<카테고리>/새-슬러그.md`를 만들고 YAML 프런트 매터를 채운 뒤 내용을 작성합니다.
3. `make static-site`를 실행해 `docs/` 아래에 `index.html`, `category/<id>/`, `post/<category>/<slug>/`가 생성되는지 확인합니다. (프로젝트 페이지라면 `BLOG_BASE_PATH=/repo/ make static-site` 사용)
4. `docs/index.html`을 브라우저로 열어 결과를 검토합니다.
5. 변경 사항을 커밋하고 GitHub Pages 브랜치에 푸시합니다.

## 3. 기본 원칙

- **폴더 = 카테고리**: 디렉터리 이름이 URL, 네비게이션 탭, 카테고리 ID로 사용됩니다.
- **메타데이터는 파일 가까이에**: 카테고리 설명/색상은 `categories.cfg`, 포스트별 정보는 각 Markdown 프런트 매터에서 유지합니다.
- **순수 정적 HTML**: `bin/bloggen`이 md4c로 바로 HTML을 생성하므로 추가 서버나 런타임이 필요 없습니다. GitHub Pages만으로 운영 가능합니다.

## 4. 다음에 읽을 문서

- [Culture](../../culture/contribution-workflow/) — 포스트 작성부터 PR까지 절차
- [Music](../../music/emscripten-material-ui/) — (히스토리) React + Wasm 파이프라인 정리
- [Software](../../software/dark-mode-highlights/) — UI/UX 하이라이트 전략

필요하면 이 문서를 복제해 다른 카테고리의 인트로를 빠르게 만들 수 있습니다. 제목, 표준 섹션(개요/절차/원칙/다음 단계)을 유지하면 독자 경험이 일관됩니다.
