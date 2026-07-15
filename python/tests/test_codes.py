"""Python tests for ``greenpeas.codes``."""

from __future__ import annotations

from pathlib import Path

import pytest
import stim

from greenpeas.codes import BBCode, SurfaceCode

DATA = Path(__file__).resolve().parents[2] / "data"

CASES = [
    (SurfaceCode, 3, "surface3.stim"),
    (SurfaceCode, 5, "surface5.stim"),
    (SurfaceCode, 7, "surface7.stim"),
    (SurfaceCode, 9, "surface9.stim"),
    (BBCode, 6, "bb6.stim"),
    (BBCode, 10, "bb10.stim"),
    (BBCode, 12, "bb12.stim"),
]


@pytest.mark.parametrize(("code_class", "distance", "circuit_name"), CASES)
def test_code_get_memory(
    code_class: type, distance: int, circuit_name: str
) -> None:
    code = code_class(distance)
    reference_path = DATA / "circuits" / circuit_name

    actual = code.get_memory(rounds=distance, p=0.001)

    expected = stim.Circuit.from_file(str(reference_path))
    
    assert actual == expected
