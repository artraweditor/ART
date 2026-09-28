"""RawPedia 코퍼스 수집기의 동작 테스트."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest


SCRIPT_PATH = Path(__file__).parents[1] / "scripts" / "fetch_rawpedia.py"
SPEC = spec_from_file_location("fetch_rawpedia", SCRIPT_PATH)
assert SPEC is not None and SPEC.loader is not None
fetch_rawpedia = module_from_spec(SPEC)
SPEC.loader.exec_module(fetch_rawpedia)


class ClassifyContentPathTests(unittest.TestCase):
    def test_includes_english_document_index(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Exposure/index.md", "---\ntitle: Exposure\n---\n\nExposure text."
        )

        self.assertEqual(decision.status, "included")
        self.assertEqual(decision.reason, "영문 문서 원문")
        self.assertEqual(decision.output_path, Path("Exposure.md"))

    def test_excludes_translated_document(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Exposure/index.de.md", "---\ntitle: Belichtung\n---\n\nText."
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "번역본 (de)")
        self.assertIsNone(decision.output_path)

    def test_excludes_hugo_section_metadata(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/_index.md", "---\ntitle: RawPedia\n---\n"
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "Hugo 섹션 메타데이터")
        self.assertIsNone(decision.output_path)

    def test_excludes_front_matter_redirect_document(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Old_Exposure/index.md", "---\nredirect_to: /exposure/\n---\n"
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "리디렉션 문서")
        self.assertIsNone(decision.output_path)

    def test_excludes_rawpedia_redirect_written_in_the_body(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Batch_Processing_Tab.md",
            "---\ntitle: Batch Processing Tab\n---\n\n1. REDIRECT [Preferences](preferences)",
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "리디렉션 문서")
        self.assertIsNone(decision.output_path)

    def test_excludes_translated_root_section_metadata(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/_index.de.md", "---\ntitle: RawPedia\n---\n"
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "번역본 (de)")
        self.assertIsNone(decision.output_path)

    def test_excludes_reviewed_management_document(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Translating_RawPedia/index.md",
            "---\ntitle: Translating RawPedia\n---\n\nContribution instructions.",
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "관리 문서: RawPedia 번역 기여 안내")
        self.assertIsNone(decision.output_path)

    def test_includes_non_redirect_english_flat_document(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/How_to_PLAY_RAW.md",
            "---\ntitle: How to PLAY RAW\n---\n\nUser documentation.",
        )

        self.assertEqual(decision.status, "included")
        self.assertEqual(decision.reason, "영문 문서 원문")
        self.assertEqual(decision.output_path, Path("How_to_PLAY_RAW.md"))

    def test_excludes_editor_lock_file(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Queue/.#index.md", "temporary editor lock"
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "편집기 잠금 파일")
        self.assertIsNone(decision.output_path)

    def test_output_path_for(self):
        self.assertEqual(
            fetch_rawpedia.output_path_for("content/Exposure/index.md"),
            Path("Exposure.md"),
        )

    def test_output_path_for_flat_document_preserves_subdirectory(self):
        self.assertEqual(
            fetch_rawpedia.output_path_for(
                "content/How_to_create_input_DCP/ICC_profiles.md"
            ),
            Path("How_to_create_input_DCP/ICC_profiles.md"),
        )

    def test_rawpedia_page_url(self):
        self.assertEqual(
            fetch_rawpedia.rawpedia_page_url("content/Exposure/index.md"),
            "https://rawpedia.rawtherapee.com/exposure/",
        )

    def test_rawpedia_page_url_for_flat_document_uses_its_filename(self):
        self.assertEqual(
            fetch_rawpedia.rawpedia_page_url("content/Batch_Processing_Tab.md"),
            "https://rawpedia.rawtherapee.com/batch_processing_tab/",
        )

    def test_git_blob_sha_matches_git_object_format(self):
        self.assertEqual(
            fetch_rawpedia.git_blob_sha(b"hello\n"),
            "ce013625030ba8dba906f756967f9e9ca394464a",
        )

    def test_manifest_records_execution_date_and_fixed_input_scope(self):
        candidate = {
            "path": "content/Exposure/index.md",
            "status": "included",
            "reason": "영문 문서 원문",
            "page_url": "https://rawpedia.rawtherapee.com/exposure/",
            "source_url": "https://github.com/RawTherapee/RawPedia/blob/abc/content/Exposure/index.md",
            "output_path": "data/rawpedia/Exposure.md",
        }

        with TemporaryDirectory() as temporary_directory:
            manifest_path = Path(temporary_directory) / "collection.md"
            fetch_rawpedia.write_manifest(
                manifest_path,
                Path("data/rawpedia"),
                "2026-09-28T00:00:00+00:00",
                "2026-09-28T00:01:00+00:00",
                "abc",
                [candidate],
            )

            manifest = manifest_path.read_text(encoding="utf-8")

        self.assertIn("- 실행일(UTC): 2026-09-28", manifest)
        self.assertIn(
            "- 원문 입력 범위: 완료 — 포함 문서 1개를 T8 입력 범위로 고정", manifest
        )


if __name__ == "__main__":
    unittest.main()
