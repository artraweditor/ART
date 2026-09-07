"""공개 원문 저장소에서 RawPedia 영문 Markdown 코퍼스를 수집한다.

RawPedia는 2025년에 MediaWiki에서 Hugo로 전환했다. 저장소는 문서별
디렉터리의 ``index.md``를 영문 원문으로, ``index.<언어>.md``를 번역본으로
관리한다. 이 수집기는 ``content/`` 아래 Markdown 전체를 후보로 기록한 뒤
영문 원문만 문서별 파일로 저장한다.
"""

import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
import re
import time
from typing import Optional

import requests


REPOSITORY = "RawTherapee/RawPedia"
BRANCH = "master"
CONTENT_PREFIX = "content/"
GITHUB_API = f"https://api.github.com/repos/{REPOSITORY}"
GITHUB_WEB = f"https://github.com/{REPOSITORY}"
RAW_CONTENT = f"https://raw.githubusercontent.com/{REPOSITORY}"
RAWPEDIA_SITE = "https://rawpedia.rawtherapee.com"
REQUEST_INTERVAL_SECONDS = 0.5
USER_AGENT = "ART-RawPedia-corpus/1.0 (+https://github.com/artpixls/ART)"
TRANSLATION_INDEX_RE = re.compile(
    r"^index\.([a-z]{2,3}(?:-[a-z0-9]+)?)\.md$", re.IGNORECASE
)


@dataclass(frozen=True)
class CollectionDecision:
    """RawPedia 콘텐츠 트리의 Markdown 파일 하나에 대한 선택 결과."""

    status: str
    reason: str
    output_path: Optional[Path]


def split_front_matter(document: str) -> tuple[str, str]:
    """YAML 의존성 없이 Hugo 프런트매터와 본문을 분리한다."""
    if not document.startswith("---"):
        return "", document

    lines = document.splitlines(keepends=True)
    if not lines or lines[0].strip() != "---":
        return "", document

    for index, line in enumerate(lines[1:], start=1):
        if line.strip() == "---":
            return "".join(lines[1:index]), "".join(lines[index + 1 :])
    return "", document


def is_redirect_only(document: str) -> bool:
    """Hugo 리디렉션 목적지만 선언한 원문인지 확인한다."""
    front_matter, body = split_front_matter(document)
    has_redirect = any(
        line.strip().lower().startswith(("redirect:", "redirect_to:"))
        for line in front_matter.splitlines()
    )
    return has_redirect and not body.strip()


def output_path_for(source_path: str) -> Path:
    """``content/Foo/index.md``를 추적 가능한 ``Foo.md``로 대응시킨다."""
    relative_directory = PurePosixPath(source_path).parent.relative_to(CONTENT_PREFIX)
    return Path(*relative_directory.parts).with_suffix(".md")


def classify_content_path(
    source_path: str, document: Optional[str] = None
) -> CollectionDecision:
    """공개된 RawPedia 영문 문서 원문만 선택한다."""
    source = PurePosixPath(source_path)
    filename = source.name

    if not source_path.startswith(CONTENT_PREFIX) or source.suffix != ".md":
        return CollectionDecision("excluded", "콘텐츠 Markdown 파일이 아님", None)
    if filename == "_index.md":
        return CollectionDecision("excluded", "Hugo 섹션 메타데이터", None)

    translation = TRANSLATION_INDEX_RE.match(filename)
    if translation:
        return CollectionDecision(
            "excluded", f"번역본 ({translation.group(1).lower()})", None
        )
    if filename != "index.md":
        return CollectionDecision("excluded", "일반 문서 페이지가 아닌 Hugo 파일", None)
    if document is not None and is_redirect_only(document):
        return CollectionDecision("excluded", "본문 없는 리디렉션 문서", None)

    return CollectionDecision(
        "included", "영문 문서 원문", output_path_for(source_path)
    )


def hugo_url_segment(segment: str) -> str:
    """원문 디렉터리에 적용되는 단순한 Hugo URL 경로 정규화를 재현한다."""
    normalized = segment.lower().replace(" ", "-")
    normalized = re.sub(r"[^a-z0-9_-]", "", normalized)
    normalized = re.sub(r"-+", "-", normalized).strip("-")
    return normalized


def rawpedia_page_url(source_path: str) -> str:
    """Hugo 문서 원문 경로에 대응하는 공개 RawPedia URL을 반환한다."""
    directory = PurePosixPath(source_path).parent.relative_to(CONTENT_PREFIX)
    parts = [hugo_url_segment(part) for part in directory.parts]
    return f"{RAWPEDIA_SITE}/{'/'.join(parts)}/"


class PacedGitHubClient:
    """요청 사이에 최소 0.5초를 두는 읽기 전용 클라이언트."""

    def __init__(self, session: Optional[requests.Session] = None) -> None:
        self.session = session or requests.Session()
        self.session.headers.update(
            {
                "Accept": "application/vnd.github+json",
                "User-Agent": USER_AGENT,
            }
        )
        self.last_request_started_at: Optional[float] = None

    def get(self, url: str) -> requests.Response:
        if self.last_request_started_at is not None:
            elapsed = time.monotonic() - self.last_request_started_at
            time.sleep(max(0.0, REQUEST_INTERVAL_SECONDS - elapsed))
        self.last_request_started_at = time.monotonic()
        response = self.session.get(url, timeout=30)
        response.raise_for_status()
        return response

    def get_json(self, url: str) -> dict:
        return self.get(url).json()

    def get_text(self, url: str) -> str:
        return self.get(url).text


def source_url_for(commit: str, source_path: str) -> str:
    return f"{GITHUB_WEB}/blob/{commit}/{source_path}"


def raw_url_for(commit: str, source_path: str) -> str:
    return f"{RAW_CONTENT}/{commit}/{source_path}"


def markdown_link(label: str, url: Optional[str]) -> str:
    return f"[{label}]({url})" if url else "—"


def write_manifest(
    manifest_path: Path,
    collected_at: str,
    commit: str,
    candidates: list[dict],
) -> None:
    """후보 목록과 원본-저장 파일 추적표를 작성한다."""
    included = sum(item["status"] == "included" for item in candidates)
    excluded = sum(item["status"] == "excluded" for item in candidates)
    pending = sum(item["status"] == "pending" for item in candidates)

    lines = [
        "# RawPedia 원문 수집 목록",
        "",
        "## 고정 수집 범위",
        "",
        f"- 실행 시각(UTC): {collected_at}",
        f"- 후보 모수: `RawTherapee/RawPedia` `content/` 아래 Markdown 파일 {len(candidates)}개",
        f"- 고정 소스 커밋: `{commit}`",
        f"- 포함 문서: {included}개",
        f"- 제외 문서: {excluded}개",
        f"- 미수집 문서: {pending}개",
        "- 저장 단위: 원본 페이지 1개당 Markdown 파일 1개",
        "- 저장 위치: `data/rawpedia/` (소스 디렉터리의 `index.md`를 대응 경로의 `.md`로 저장)",
        f"- 업스트림: {GITHUB_WEB}",
        "",
        "RawPedia는 2025년에 MediaWiki에서 Hugo/Markdown으로 전환되었다. 이 목록은 "
        "공개 원문 저장소의 고정 커밋을 후보 모수로 사용하며, 원본 페이지 URL과 "
        "소스 스냅샷 URL을 모두 남긴다.",
        "",
        "## 선택 규칙",
        "",
        "- 포함: `content/**/index.md`의 영어 본문 문서.",
        "- 제외: `index.{언어코드}.md` 번역본, Hugo `_index.md` 섹션/목록 메타데이터, "
        "리디렉션 대상만 선언하고 본문이 없는 문서, 일반 문서가 아닌 Markdown 파일.",
        "- 리디렉션은 문서 원문이 없는 경우에만 제외한다. 본문이 있는 문서의 Hugo alias는 "
        "원문 페이지의 이전 URL일 수 있으므로 제외 근거로 사용하지 않는다.",
        f"- HTTP 요청 간 최소 간격: {REQUEST_INTERVAL_SECONDS:.1f}초.",
        "- 이 수집은 원문만 보관하며 청킹·임베딩·범위 축소를 수행하지 않는다.",
        "",
        "## 후보별 결과",
        "",
        "| 후보 소스 경로 | 결과 | 근거 | RawPedia 원본 페이지 | 소스 스냅샷 | 저장 Markdown |",
        "|---|---|---|---|---|---|",
    ]

    for item in candidates:
        lines.append(
            "| `{path}` | {status} | {reason} | {page} | {source} | {output} |".format(
                path=item["path"],
                status=item["status"],
                reason=item["reason"],
                page=markdown_link("page", item["page_url"]),
                source=markdown_link("source", item["source_url"]),
                output=(f"`{item['output_path']}`" if item["output_path"] else "—"),
            )
        )

    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def fetch_corpus(outdir: Path, manifest_path: Path) -> int:
    """RawPedia 영문 원문 전체를 내려받고 수집 목록을 작성한다."""
    client = PacedGitHubClient()
    commit_data = client.get_json(f"{GITHUB_API}/commits/{BRANCH}")
    commit = commit_data["sha"]
    tree_sha = commit_data["commit"]["tree"]["sha"]
    tree_data = client.get_json(f"{GITHUB_API}/git/trees/{tree_sha}?recursive=1")
    if tree_data.get("truncated"):
        raise RuntimeError(
            "RawPedia Git 트리 응답이 잘려 있어 불완전한 범위는 저장하지 않습니다."
        )

    candidate_paths = sorted(
        item["path"]
        for item in tree_data["tree"]
        if item["type"] == "blob"
        and item["path"].startswith(CONTENT_PREFIX)
        and item["path"].endswith(".md")
    )
    candidates = []
    for source_path in candidate_paths:
        decision = classify_content_path(source_path)
        page_url = rawpedia_page_url(source_path)
        source_url = source_url_for(commit, source_path)
        output_path = None

        if decision.status == "included":
            planned_output = decision.output_path
            destination = (outdir / planned_output) if planned_output else None
            try:
                if destination and destination.exists() and destination.stat().st_size > 0:
                    document = destination.read_text(encoding="utf-8")
                else:
                    document = client.get_text(raw_url_for(commit, source_path))
                decision = classify_content_path(source_path, document)
                if decision.status == "included":
                    output_path = decision.output_path
                    destination = outdir / output_path
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    destination.write_text(document, encoding="utf-8")
            except requests.RequestException as error:
                decision = CollectionDecision(
                    "pending", f"다운로드 실패: {error.__class__.__name__}", None
                )

        candidates.append(
            {
                "path": source_path,
                "status": decision.status,
                "reason": decision.reason,
                "page_url": page_url,
                "source_url": source_url,
                "output_path": str(Path("data/rawpedia") / output_path)
                if output_path
                else None,
            }
        )

    collected_at = datetime.now(timezone.utc).replace(microsecond=0).isoformat()
    write_manifest(manifest_path, collected_at, commit, candidates)
    pending = sum(item["status"] == "pending" for item in candidates)
    print(
        f"후보: {len(candidates)}개; 포함: "
        f"{sum(item['status'] == 'included' for item in candidates)}개; "
        f"제외: {sum(item['status'] == 'excluded' for item in candidates)}개; "
        f"미수집: {pending}개"
    )
    print(f"수집 목록: {manifest_path}")
    return pending


def main() -> None:
    parser = argparse.ArgumentParser(
        description="RawPedia Hugo 영문 원문 코퍼스 전체를 수집합니다."
    )
    parser.add_argument(
        "--outdir",
        "-o",
        type=Path,
        default=Path("data/rawpedia"),
        help="Markdown 저장 디렉터리",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("docs/rawpedia_collection.md"),
        help="후보 목록과 출처를 기록할 문서",
    )
    args = parser.parse_args()

    pending = fetch_corpus(args.outdir, args.manifest)
    if pending:
        raise SystemExit("미수집 문서가 있습니다. 수집 목록에서 경로를 확인하세요.")


if __name__ == "__main__":
    main()
