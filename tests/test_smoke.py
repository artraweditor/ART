"""Minimal smoke coverage for the pipeline package."""

from pathlib import Path
import sys


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))


def test_artagent_package_imports() -> None:
    import artagent

    assert artagent.__name__ == "artagent"
