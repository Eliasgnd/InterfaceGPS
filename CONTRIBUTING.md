# Contributing to InterfaceGPS

Thank you for your interest in contributing.

## How to contribute

1. Fork the repository.
2. Create a feature branch from `main`.
3. Keep changes focused and documented.
4. Run relevant checks locally (build/tests/doc generation).
5. Open a Pull Request with a clear description.

## Development setup

```bash
qmake6 InterfaceGPS.pro
make -j"$(nproc)"
```

## Coding guidelines

- Keep compatibility with Qt 6 and qmake.
- Do not remove existing functional features.
- Prefer incremental, readable changes over large refactors.
- Add/update Doxygen comments for non-trivial classes and methods.

## Pull Request expectations

- Explain the problem and the proposed solution.
- Mention risks and impacted modules.
- Include screenshots for UI changes when relevant.
- Update documentation (`README`, `docs/`, `CHANGELOG`) if needed.

## Reporting issues

Please use the issue templates in `.github/ISSUE_TEMPLATE/`.
