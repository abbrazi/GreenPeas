"""Stabilizer QEC codes (mirrors ``GreenPeas/QEC/Codes``)."""

from greenpeas._data_path import data_path
from greenpeas._gppy.codes import _BBCode, _SurfaceCode

__all__ = ["BBCode", "SurfaceCode"]


class SurfaceCode(_SurfaceCode):
    def __init__(self, distance: int) -> None:
        super().__init__(distance, data_path())


class BBCode(_BBCode):
    def __init__(self, distance: int) -> None:
        super().__init__(distance, data_path())
