"""공개 원문 저장소에서 RawPedia 영문 Markdown 코퍼스를 수집한다.

RawPedia는 2025년에 MediaWiki에서 Hugo로 전환했다. ``content/`` 아래의
Markdown 전체를 후보로 남기고, 리디렉션·번역·관리 문서·편집기 잠금 파일을
제외한 영문 원문을 페이지별 Markdown 파일로 저장한다.
"""

import argparse
from dataclasses import dataclass
from datetime import datetime, timezone
import hashlib
from pathlib import Path, PurePosixPath
import re
import time
from typing import Optional
from urllib.parse import quote

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
TRANSLATION_FILENAME_RE = re.compile(
    r"^(?:_?index)\.([a-z]{2,3}(?:-[a-z0-9]+)?)\.md$", re.IGNORECASE
)
BODY_REDIRECT_RE = re.compile(r"^\s*(?:\d+\.\s*)?REDIRECT\b", re.IGNORECASE | re.MULTILINE)

# 원문을 확인해 관리자·기여 안내임을 판정한 경로만 명시적으로 제외한다.
MANAGEMENT_DOCUMENT_REASONS = {
    "content/Coding_Rawpedia_pages.md": "관리 문서: RawPedia 작성 규칙",
    "content/Contributing.md": "관리 문서: 기여 안내",
    "content/How_to_Coverity.md": "관리 문서: 정적 분석 운영 절차",
    "content/How_to_release_RawTherapee.md": "관리 문서: 릴리스 절차",
    "content/RawPedia_Book.md": "관리 문서: RawPedia 책 생성 절차",
    "content/changes.md": "관리 문서: 변경 이력",
    "content/Translating_RawPedia/index.md": "관리 문서: RawPedia 번역 기여 안내",
    "content/Translating_RawTherapee/index.md": "관리 문서: RawTherapee 번역 기여 안내",
}

# 이전 MediaWiki 원문 중 파일명과 내용을 확인해 비영문임을 판정한 파일이다.
NON_ENGLISH_DOCUMENT_REASONS = {
    "content/Bordi_e_Microcrontasto.md": "비영문 문서: 이탈리아어",
    "content/Creare_profili_di_elaborazione_per_uso_generale.md": "비영문 문서: 이탈리아어",
    "content/Nitidezza.md": "비영문 문서: 이탈리아어",
    "content/Profili_di_elaborazione_dinamici.md": "비영문 문서: 이탈리아어",
    "content/Riduzione_Rumore_Puntuale.md": "비영문 문서: 이탈리아어",
    "content/Sidecar_Files_-_Profili_di_sviluppo.md": "비영문 문서: 이탈리아어",
    "content/Wavelets/pv.md": "비영문 문서: 프랑스어",
}


@dataclass(frozen=True)
class CollectionDecision:
    """RawPedia 콘텐츠 트리의 Markdown 파일 하나에 대한 선택 결과."""

    status: str
    reason: str
    output_path: Optional[Path]


def utc_now() -> str:
    """수집 기록에 사용할 초 단위 UTC 시각을 반환한다."""
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


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


def is_redirect_document(document: str) -> bool:
    """프런트매터 또는 기존 RawPedia 본문의 리디렉션 표기를 찾는다."""
    front_matter, body = split_front_matter(document)
    has_front_matter_redirect = any(
        line.strip().lower().startswith(("redirect:", "redirect_to:"))
        for line in front_matter.splitlines()
    )
    return has_front_matter_redirect or bool(BODY_REDIRECT_RE.search(body))


def output_path_for(source_path: str) -> Path:
    """원문 경로를 페이지별 로컬 Markdown 경로로 대응시킨다."""
    relative = PurePosixPath(source_path).relative_to(CONTENT_PREFIX)
    if relative.name == "index.md":
        return Path(*relative.parent.parts).with_suffix(".md")
    return Path(*relative.parts)


def classify_content_path(
    source_path: str, document: Optional[str] = None
) -> CollectionDecision:
    """영문 본문·번역·리디렉션·관리 후보를 결정 가능한 규칙으로 분류한다."""
    source = PurePosixPath(source_path)
    filename = source.name

    if not source_path.startswith(CONTENT_PREFIX) or source.suffix != ".md":
        return CollectionDecision("excluded", "콘텐츠 Markdown 파일이 아님", None)

    translation = TRANSLATION_FILENAME_RE.match(filename)
    if translation:
        return CollectionDecision(
            "excluded", f"번역본 ({translation.group(1).lower()})", None
        )
    if filename == "_index.md":
        return CollectionDecision("excluded", "Hugo 섹션 메타데이터", None)
    if filename.startswith(".#"):
        return CollectionDecision("excluded", "편집기 잠금 파일", None)
    if source_path in MANAGEMENT_DOCUMENT_REASONS:
        return CollectionDecision(
            "excluded", MANAGEMENT_DOCUMENT_REASONS[source_path], None
        )
    if source_path in NON_ENGLISH_DOCUMENT_REASONS:
        return CollectionDecision(
            "excluded", NON_ENGLISH_DOCUMENT_REASONS[source_path], None
        )
    if document is not None and is_redirect_document(document):
        return CollectionDecision("excluded", "리디렉션 문서", None)

    return CollectionDecision(
        "included", "영문 문서 원문", output_path_for(source_path)
    )


def hugo_url_segment(segment: str) -> str:
    """원문 경로에 적용되는 단순한 Hugo URL 경로 정규화를 재현한다."""
    normalized = segment.lower().replace(" ", "-")
    normalized = re.sub(r"[^a-z0-9_-]", "", normalized)
    normalized = re.sub(r"-+", "-", normalized).strip("-")
    return normalized


def rawpedia_page_url(source_path: str) -> Optional[str]:
    """원문 경로에 대응하는 RawPedia 공개 페이지 URL을 반환한다."""
    relative = PurePosixPath(source_path).relative_to(CONTENT_PREFIX)
    filename = relative.name
    if filename.startswith(".#"):
        return None
    if filename == "index.md" or TRANSLATION_FILENAME_RE.match(filename):
        route_parts = relative.parent.parts
    elif filename == "_index.md":
        route_parts = ()
    else:
        route_parts = relative.with_suffix("").parts

    normalized_parts = [hugo_url_segment(part) for part in route_parts]
    path = "/".join(part for part in normalized_parts if part)
    return f"{RAWPEDIA_SITE}/{path}/" if path else f"{RAWPEDIA_SITE}/"


def git_blob_sha(content: bytes) -> str:
    """Git tree의 blob SHA와 비교할 수 있는 SHA-1을 계산한다."""
    header = f"blob {len(content)}\0".encode("ascii")
    return hashlib.sha1(header + content).hexdigest()


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

    def get_bytes(self, url: str) -> bytes:
        return self.get(url).content


def source_url_for(commit: str, source_path: str) -> str:
    """고정 커밋의 GitHub 원문 보기 URL을 반환한다."""
    return f"{GITHUB_WEB}/blob/{commit}/{quote(source_path, safe='/')}"


def raw_url_for(commit: str, source_path: str) -> str:
    """고정 커밋의 원문 바이트 URL을 반환한다."""
    return f"{RAW_CONTENT}/{commit}/{quote(source_path, safe='/')}"


def get_source_bytes(
    client: PacedGitHubClient,
    destination: Path,
    blob_sha: str,
    source_url: str,
) -> tuple[bytes, bool]:
    """현재 Git blob과 일치하는 로컬 원문만 재사용하고 나머지는 다시 받는다."""
    if destination.is_file():
        local_bytes = destination.read_bytes()
        if git_blob_sha(local_bytes) == blob_sha:
            return local_bytes, True

    downloaded = client.get_bytes(source_url)
    if git_blob_sha(downloaded) != blob_sha:
        raise ValueError("다운로드한 원문이 Git tree의 blob SHA와 일치하지 않습니다.")
    return downloaded, False


def markdown_link(label: str, url: Optional[str]) -> str:
    return f"[{label}]({url})" if url else "—"


def write_manifest(
    manifest_path: Path,
    outdir: Path,
    candidate_checked_at: str,
    collection_completed_at: str,
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
        f"- 실행일(UTC): {candidate_checked_at[:10]}",
        f"- 후보 확인 시각(UTC): {candidate_checked_at}",
        f"- 수집 완료 시각(UTC): {collection_completed_at}",
        f"- 후보 모수: `RawTherapee/RawPedia` `content/` 아래 Markdown 파일 {len(candidates)}개",
        f"- 고정 소스 커밋: `{commit}`",
        f"- 최종 문서: {included}개",
        f"- 원문 입력 범위: 완료 — 포함 문서 {included}개를 T8 입력 범위로 고정",
        f"- 제외 문서: {excluded}개",
        f"- 미수집 문서: {pending}개",
        "- 저장 단위: 원본 페이지 1개당 Markdown 파일 1개",
        f"- 저장 위치: `{outdir}`",
        f"- 업스트림: {GITHUB_WEB}",
        "",
        "RawPedia 공개 원문 저장소의 고정 커밋을 후보 모수로 사용한다. 각 후보는 "
        "RawPedia 공개 페이지와 GitHub 원문 스냅샷으로 추적한다.",
        "",
        "## 선택 규칙",
        "",
        "- 포함: 영문 본문 Markdown. 디렉터리형 `index.md`와 평면 `.md` 모두를 후보별로 판정한다.",
        "- 제외: `index.{언어코드}.md` 및 `_index.{언어코드}.md` 번역본, Hugo `_index.md` 메타데이터, "
        "편집기 잠금 파일, 리디렉션 문서, 원문 확인을 거친 관리·기여 문서와 비영문 레거시 문서.",
        "- 리디렉션: Hugo 프런트매터의 `redirect`/`redirect_to`와 본문의 `REDIRECT` 표기를 모두 제외한다.",
        "- 재실행: 기존 파일은 현재 Git tree의 blob SHA와 일치할 때만 재사용한다.",
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

    candidate_items = sorted(
        (
            item
            for item in tree_data["tree"]
            if item["type"] == "blob"
            and item["path"].startswith(CONTENT_PREFIX)
            and item["path"].endswith(".md")
        ),
        key=lambda item: item["path"],
    )
    candidate_checked_at = utc_now()
    candidates = []
    for item in candidate_items:
        source_path = item["path"]
        blob_sha = item["sha"]
        decision = classify_content_path(source_path)
        page_url = rawpedia_page_url(source_path)
        source_url = source_url_for(commit, source_path)
        output_path = None

        if decision.status == "included":
            planned_output = decision.output_path
            assert planned_output is not None
            destination = outdir / planned_output
            try:
                source_bytes, reused = get_source_bytes(
                    client, destination, blob_sha, raw_url_for(commit, source_path)
                )
                document = source_bytes.decode("utf-8")
                decision = classify_content_path(source_path, document)
                if decision.status == "included":
                    output_path = decision.output_path
                    assert output_path is not None
                    destination = outdir / output_path
                    if not reused:
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        destination.write_bytes(source_bytes)
            except (requests.RequestException, UnicodeDecodeError, ValueError) as error:
                decision = CollectionDecision(
                    "pending", f"원문 확인 실패: {error.__class__.__name__}", None
                )

        candidates.append(
            {
                "path": source_path,
                "status": decision.status,
                "reason": decision.reason,
                "page_url": page_url,
                "source_url": source_url,
                "output_path": str(outdir / output_path) if output_path else None,
            }
        )

    collection_completed_at = utc_now()
    write_manifest(
        manifest_path,
        outdir,
        candidate_checked_at,
        collection_completed_at,
        commit,
        candidates,
    )
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
