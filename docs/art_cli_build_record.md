# ART-cli Build Record

## 2026-09-02

| Item | Value |
| --- | --- |
| Build commit | `d0ec726f027eb1d338a4ae808ba8be1650bb35fe` |
| PPVERSION | `1045` |
| Input RAW | `data/fivek_sample/sample1.nef` |
| Processing profile | `data/test_profile.arp` |
| Renderer | `build/rtgui/ART-cli` |

### Commands and result

```bash
rm -rf build
./scripts/build_art_cli.sh
./scripts/test_art_cli.sh
```

The build completed successfully, and `test_art_cli.sh` exited with status 0
after creating `data/fivek_sample/sample1.jpg`.
