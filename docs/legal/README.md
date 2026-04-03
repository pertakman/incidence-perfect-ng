# Legal Notes

## Code License

Unless stated otherwise, source code in this repository is licensed under the MIT License.

See:

- `LICENSE`

## Documentation And Assets

Repository-authored documentation files (`.md`, `.txt`, release notes, validation logs) are provided under the same MIT terms unless a specific file says otherwise.

Repository-authored generated release artifacts may also be tracked when intentionally published by the project, for example:

- manual PDFs under `docs/manual/releases/`

Generated working files in ignored build-output folders do not change the licensing position by themselves.

## Third-Party Content

Some materials in this repository are third-party references and are **not** re-licensed by this project. Examples include:

- vendor hardware manuals/schematics in `docs/hardware/`
- external libraries pulled via PlatformIO (see `platformio.ini`)
- third-party tools used to build documentation (for example `pandoc` or local browser PDF engines)

Those materials remain under their original licenses/terms from their respective owners.

Repository practice:

- Third-party vendor binaries are kept out of Git when possible.
- Use `docs/hardware/SOURCES.md` for provenance links + checksums.

## Trademarks

Any product names, vendor names, and logos referenced in this repository are the property of their respective owners.
