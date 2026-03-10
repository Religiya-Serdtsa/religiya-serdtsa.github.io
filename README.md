# cwist Material UI GitHub Blog

이 저장소는 **Markdown + md4c** 파이프라인을 사용해 정적 HTML을 생성하고 GitHub Pages에 배포하는 블로그입니다. 각 카테고리는 INI 형식(`categories.cfg`)으로 정의되며, 각 Markdown 파일은 YAML 프런트 매터(제목/날짜/태그 등)를 포함해 Jekyll과 비슷한 워크플로를 제공합니다.

## 구성 요소

- **정적 생성기**: `bin/bloggen` 바이너리가 `categories.cfg`와 `posts/<카테고리>/*.md`를 읽어 `docs/` 이하에 `index.html`, `category/<id>/index.html`, `post/<category>/<slug>/index.html`을 생성합니다.
- **자산**: `assets/styles.css`가 자동으로 `docs/assets/styles.css`로 복사되어 모든 페이지가 동일한 테마를 공유합니다.
- **콘텐츠 구조**: 카테고리마다 폴더를 만들고 Markdown에 프런트 매터(`title`, `date`, `excerpt`, `tags`, `reading_minutes`)를 추가하면 됩니다.

## 빌드 방법

```bash
make static-site                             # docs/ 이하에 정적 HTML 생성
```

### GitHub Actions 배포

`.github/workflows/deploy.yml`은 `main` 브랜치에 푸시되면 `make static-site`를 실행하고 `docs/` 폴더를 GitHub Pages에 업로드합니다. 별도의 서버 비용 없이 Pages만으로 배포가 끝납니다.

## 새 글 작성 순서

1. `categories.cfg`에 새 카테고리 메타데이터를 추가하거나 기존 항목을 수정합니다.
2. `posts/<카테고리>/새-슬러그.md` 파일을 만들고 YAML 프런트 매터(`title`, `date`, `excerpt`, `tags`, `reading_minutes`)를 선언한 뒤 본문을 작성합니다.
3. `make static-site`를 실행하면 `docs/`에 완성된 HTML이 생성됩니다.
4. 변경 사항을 커밋하고 GitHub Pages가 설정된 브랜치에 푸시합니다.
