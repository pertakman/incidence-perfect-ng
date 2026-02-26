from datetime import datetime
import os
import subprocess

Import("env")


def month_commit_count(project_dir: str, year: int, month: int) -> int:
    since = f"{year}-{month:02d}-01 00:00:00"
    cmd = ["git", "rev-list", "--count", f"--since={since}", "HEAD"]
    try:
        out = subprocess.check_output(cmd, cwd=project_dir, text=True).strip()
        count = int(out)
        return count if count > 0 else 1
    except Exception:
        return 1


override = os.getenv("FW_VERSION_OVERRIDE", "").strip()
if override:
    version = override
else:
    now = datetime.now()
    x = month_commit_count(env.subst("$PROJECT_DIR"), now.year, now.month)
    version = f"{now.year}.{now.month}.{x}"

env.Append(CPPDEFINES=[("FW_VERSION", f"\\\"{version}\\\"")])
print(f"[fw-version] FW_VERSION={version}")
