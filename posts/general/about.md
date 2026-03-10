---
title: Religiya Serdtsa 블로그 안내 — 공지, 소개, 전체 안내
date: 2026-03-10
excerpt: 공지, 소개, 전체 블로그 안내
tags: Welcome, Guide, Overview, General
reading_minutes: 3
---

# Religiya Serdtsa 블로그 안내

**General** 카테고리는 블로그 전체에 걸친 **공지**, **소개**, **전체 안내**를 담습니다.

## 블로그 구조

| 카테고리 | 설명 |
|----------|------|
| Software | 개발 도구, UI/UX, 알고리즘, Punkware 소개 |
| Post | 블로그 운영 공지 |
| General | 공지, 소개, 전체 블로그 안내 (이 카테고리) |
| Music | 음악과 문화, 펑크부터 바로크까지 |
| Culture | 세계의 좋은 취향과 문화들 |

## 기술 스택

이 블로그는 순수 C로 작성된 정적 제너레이터 `bin/bloggen`이 구동합니다.

- **cwist**: C 프레임워크 — 문자열(`sstring`), 오류 처리 제공
- **md4c**: 고속 CommonMark 파서 (C 구현)
- **GitHub Pages**: 정적 HTML 서빙

## 빠른 시작

1. `categories.cfg`에 카테고리 메타데이터를 선언합니다.
2. `posts/<카테고리>/슬러그.md`를 YAML 프런트 매터와 함께 작성합니다.
3. `make static-site`를 실행하면 `docs/`에 완성된 HTML이 생성됩니다.
4. 커밋 후 푸시하면 GitHub Actions가 자동으로 배포합니다.
