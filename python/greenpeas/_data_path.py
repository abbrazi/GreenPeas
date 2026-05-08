"""Package data location helpers."""

from pathlib import Path


def data_path() -> str:
    """Return the installed package data root."""
    return str(Path(__file__).resolve().parent / "data")
