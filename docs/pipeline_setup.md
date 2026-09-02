# Pipeline Setup Record

## Test execution

- Python: `/Users/user/Workspace/Programming/Projects/AAART/venv/bin/python`
  (Python 3.14)
- Command: `/Users/user/Workspace/Programming/Projects/AAART/venv/bin/python -m pytest`

## Dependency installation record

| Dependency | Version | Status | First use |
| --- | --- | --- | --- |
| pytest | 9.1.1 | Installed 2026-09-02 | Pipeline smoke test |
| requests | 2.34.2 | Already installed | RawPedia collection |
| beautifulsoup4 | 4.15.0 | Already installed | RawPedia collection |
| markdownify | 1.2.3 | Already installed | RawPedia collection |

Add a dependency here when its consuming task installs it. Packaging metadata
will be considered only when the installed set requires a maintained list.
