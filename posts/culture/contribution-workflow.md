---
title: How to Contribute
date: 2024-05-25
excerpt: 깃허브 블로그에 새 글이나 기능을 추가하는 표준 작업 절차를 정리했습니다.
tags: Contribution, Workflow, Guide
reading_minutes: 5
---

# How to Contribute

> **Goal** — 깃허브 블로그에 새 글이나 기능을 추가하는 표준 작업 절차를 정의합니다.

## 1. 준비 체크리스트

- [ ] `categories.cfg`와 `posts/` 최신 상태 확인
- [ ] 브랜치 명명 규칙 `feature/<slug>` 또는 `fix/<topic>` 준수
- [ ] `make static-site`가 로컬에서 정상 동작하는지 확인

## 2. 문서 작성 단계

1. **카테고리 선택**: 없으면 `categories.cfg`에 새 섹션을 추가하고 `posts/<새카테고리>/` 폴더를 만듭니다.
2. **Markdown 작성**: 파일 상단에 YAML 프런트 매터(`title`, `date`, `excerpt`, `tags`, `reading_minutes`)를 선언하고 본문을 작성합니다.
3. **자산 배치**: 이미지나 추가 리소스가 있으면 같은 폴더에 저장하고, 상대 경로로 참조합니다.
4. **정적 사이트 생성**: `make static-site`(필요하면 `BLOG_BASE_PATH=/repo/ make static-site`)를 실행해 HTML이 재생성되는지 확인합니다.
5. **링크 검증**: 생성된 `docs/index.html`, `docs/category/<id>/index.html`, `docs/post/<category>/<slug>/index.html`을 브라우저에서 열어 내비게이션·본문·스타일을 확인합니다.

## 3. 코드/테마 변경 단계

1. `assets/styles.css`를 수정했다면 대비 전·후 스크린샷을 PR에 첨부합니다.
2. 카테고리 색상이나 설명을 바꿨다면 `categories.cfg` diff를 꼼꼼히 검토합니다.
3. Markdown 프런트 매터 양식을 변경했다면 README 및 다른 문서에도 반영합니다.

## 4. PR 체크리스트

- [ ] `npm` 의존성이 없으므로 `package-lock.json` 등은 커밋하지 않음
- [ ] `categories.cfg`에 누락된 항목이 없는지 확인
- [ ] 새 카테고리/포스트가 생성된 HTML(`docs/category/*`, `docs/post/*`)까지 포함됐는지 확인
- [ ] `README.md`에 필요한 설명을 추가했는지 다시 확인

## 5. 승인 이후

- `main` 또는 Pages 소스 브랜치에 squash merge
- GitHub Pages 빌드 상태를 확인하고, 이슈에 완료 코멘트를 남깁니다.
- 필요하다면 General 카테고리에 “무엇이 바뀌었는지” 요약 포스트를 작성해 독자에게 안내합니다.

이 워크플로를 참고하면 블로그 추가 작업이 예측 가능하고, 누구나 동일한 UX를 유지하면서 콘텐츠를 기여할 수 있습니다.
