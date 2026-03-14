import os
import shutil
from pathlib import Path

Import("env")

if env.IsIntegrationDump():
    Return()


def build_candidate_paths(project_dir: Path):
    env_override = os.getenv("QMI8658_LIB_PATH", "").strip()
    candidates = []
    if env_override:
        candidates.append(Path(env_override).expanduser())
    candidates.extend([
        project_dir / "lib" / "QMI8658",
        Path("C:/dev/Arduino/Libraries/QMI8658"),
        Path.home() / "Documents" / "Arduino" / "libraries" / "QMI8658",
    ])
    return candidates


def normalize(path: Path) -> Path:
    try:
        return path.resolve()
    except Exception:
        return path


project_dir = Path(env.subst("$PROJECT_DIR"))
dest_dir = project_dir / ".pio" / "local-libs" / "QMI8658"
candidates = build_candidate_paths(project_dir)
seen = set()

for candidate in candidates:
    path = normalize(candidate)
    key = str(path).lower()
    if key in seen:
        continue
    seen.add(key)
    if path.is_dir():
        if dest_dir.exists():
            shutil.rmtree(dest_dir)
        shutil.copytree(
            path,
            dest_dir,
            ignore=shutil.ignore_patterns(".git", ".svn", ".hg", "__pycache__", "examples")
        )
        print(f"[qmi8658] Synced library from {path} to {dest_dir}")
        break
else:
    search_lines = "\n".join(f"  - {normalize(path)}" for path in candidates)
    raise FileNotFoundError(
        "QMI8658 library not found.\n"
        "Set QMI8658_LIB_PATH to your local library path, or place the library in one of:\n"
        f"{search_lines}"
    )
