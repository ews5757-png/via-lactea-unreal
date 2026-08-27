# Via Lactea — LDJ Contributions

Unreal Engine 5 팀 프로젝트 **Via Lactea**에서 제가 단독으로 작성·수정한 C++ 파일만 선별한 이력서·포트폴리오용 저장소입니다.

This portfolio snapshot contains only C++ source files whose complete SVN change history is exclusively attributed to my account, `LDJ`.

## 담당 영역 / Contributions

- 플레이어 전투용 Animation Notify 및 Notify State
- 공격 충돌, 방어 구간, 무기 IK, 무기 트레일 처리
- 캐릭터 애니메이션 인스턴스와 이동 상태 정의
- 무기·장비 계층과 검, 방패, 활, 해머, 대검, 채집 도구 구현
- 화살 투사체 및 활 애니메이션 처리

## SVN 검증 기준 / SVN verification

원격 SVN 전체 로그 144개 리비전을 조회해 `LDJ` 커밋 38개를 분석했습니다. 현재 포함된 38개 파일은 다음 조건을 모두 만족합니다.

- SVN에서 최초 추가자가 `LDJ`
- 파일의 전체 변경 작성자가 `LDJ` 한 명뿐임
- 현재 팀 프로젝트의 `Source/`에 존재하는 C++ 또는 헤더 파일

공동 수정 파일은 제가 최초 작성했거나 많은 부분을 담당했더라도 공개본에서 제외했습니다. `.svn`, 팀 공용 설정, 에셋, 플러그인, 바이너리와 캐시도 포함하지 않았습니다.

The complete remote SVN history was inspected across 144 revisions and 38 commits by `LDJ`. Every included file was created by `LDJ` and has no other author in its file-level SVN history. All shared files and non-source project data are excluded.

## 저장소 범위 / Scope

이 저장소는 기여 코드 검토용 발췌본이며 전체 게임을 빌드하거나 실행하기 위한 완전한 Unreal 프로젝트가 아닙니다. 일부 코드는 공개하지 않은 팀 공용 타입을 참조합니다.

This is a code-review snapshot rather than a complete buildable Unreal project. Some files reference shared team types that are intentionally not redistributed.

## Technologies

- C++
- Unreal Engine 5
- Animation Notify / Animation Instance
- Component-based gameplay architecture
- SVN team workflow

## License

Portfolio review only. No license is granted for reuse or redistribution.

