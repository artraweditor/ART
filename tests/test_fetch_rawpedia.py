"""RawPedia 코퍼스 수집기의 동작 테스트."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
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

    def test_excludes_redirect_only_document(self):
        decision = fetch_rawpedia.classify_content_path(
            "content/Old_Exposure/index.md", "---\nredirect_to: /exposure/\n---\n"
        )

        self.assertEqual(decision.status, "excluded")
        self.assertEqual(decision.reason, "본문 없는 리디렉션 문서")
        self.assertIsNone(decision.output_path)

    def test_output_path_for(self):
        self.assertEqual(
            fetch_rawpedia.output_path_for("content/Exposure/index.md"),
            Path("Exposure.md"),
        )

    def test_rawpedia_page_url(self):
        self.assertEqual(
            fetch_rawpedia.rawpedia_page_url("content/Exposure/index.md"),
            "https://rawpedia.rawtherapee.com/exposure/",
        )


if __name__ == "__main__":
    unittest.main()
