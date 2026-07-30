"""Python tests for ``greenpeas.error_analysis``."""

from __future__ import annotations

from pathlib import Path

import pytest
import stim

from greenpeas.codes import SurfaceCode
from greenpeas.error_analysis import CorrelationLevel, get_driver

DATA = Path(__file__).resolve().parents[2] / "data"

CASES = [
    (SurfaceCode, 3, "surface3"),
    (SurfaceCode, 5, "surface5"),
]

CORRELATION_LEVELS = [
    CorrelationLevel.L0,
    CorrelationLevel.L1,
    CorrelationLevel.L2,
]


@pytest.mark.parametrize(("code_class", "distance", "name"), CASES)
@pytest.mark.parametrize("level", CORRELATION_LEVELS)
def test_driver_compile_detector_error_model(
    code_class: type,
    distance: int,
    name: str,
    level: CorrelationLevel,
) -> None:
    code = code_class(distance)
    reference_path = DATA / "dems" / f"{name}_{level.name}.dem"

    circuit = code.get_memory(rounds=distance, p=0.001)

    driver = get_driver(circuit, correlation_level=level)
    actual = driver.compile_detector_error_model(circuit)

    expected = stim.DetectorErrorModel.from_file(str(reference_path))

    assert actual == expected


@pytest.mark.parametrize("level", CORRELATION_LEVELS)
def test_driver_reuse(level: CorrelationLevel) -> None:
    """A driver sized for d5 can compile a d3 circuit."""
    d5_circuit = SurfaceCode(5).get_memory(rounds=5, p=0.001)
    d3_circuit = SurfaceCode(3).get_memory(rounds=3, p=0.001)
    reference_path = DATA / "dems" / f"surface3_{level.name}.dem"

    driver = get_driver(d5_circuit, correlation_level=level)
    actual = driver.compile_detector_error_model(d3_circuit)

    expected = stim.DetectorErrorModel.from_file(str(reference_path))
    assert actual == expected
