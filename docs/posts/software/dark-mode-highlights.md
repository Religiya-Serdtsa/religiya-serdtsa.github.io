---
title: 주황색 하이라이트와 다크 모드 UX
date: 2024-05-24
excerpt: 토글 UX와 하이라이트 색 체계를 정리했습니다.
tags: Design, UX, Theme
reading_minutes: 2
---

# 주황색 하이라이트와 다크 모드 UX

웹 접근성을 고려하면 단순히 다크 모드를 켜는 것 이상이 필요합니다. 이번 테마는 다음 요소를 갖습니다.

- `prefers-color-scheme`를 감지해 초기 모드를 정합니다.
- 사용자의 선택은 `localStorage`에 저장합니다.
- 모든 `code`, `mark`, `::selection` 컬러는 `#ff7a18` 그라데이션을 기반으로 통합했습니다.
- Material UI `Paper`와 `Card` 그림자를 줄여 GitHub Pages에서도 부드럽게 보입니다.

```js
const theme = React.useMemo(() => createTheme({
  palette: {
    mode,
    primary: {
      main: '#ff7a18',
      contrastText: '#111'
    },
    background: {
      default: mode === 'dark' ? '#0f1116' : '#f4f6fb',
      paper: mode === 'dark' ? '#151821' : '#ffffff'
    }
  },
  shape: {
    borderRadius: 16
  }
}), [mode]);
```

모드 전환 스위치는 AppBar 우측에 배치되어 있으며, 키보드 접근성을 위해 `aria-label`과 포커스 링을 명시했습니다. 사용자는 코드 블록과 체크리스트 등 모든 Markdown 요소에서 동일한 하이라이트를 경험할 수 있습니다.
