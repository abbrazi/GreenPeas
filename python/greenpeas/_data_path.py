"""Package data location helpers."""

from pathlib import Path


def data_path() -> str:
    """Return the package data root, falling back to the repo ``data/`` tree."""
    package_data = Path(__file__).resolve().parent / "data"
    if package_data.is_dir():
        return str(package_data)

    repo_data = Path(__file__).resolve().parents[2] / "data"
    if repo_data.is_dir():
        return str(repo_data)

    return str(package_data)
