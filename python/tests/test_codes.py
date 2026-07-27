"""Python tests for ``greenpeas.codes``."""

from __future__ import annotations

from pathlib import Path

import numpy as np
import pytest
import stim

from greenpeas.codes import (
    BBCode,
    ConcatenatedSurfaceCode,
    MeasurementStrategy,
    SurfaceCode,
)

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

CSF_CASES = [
    (4, MeasurementStrategy.Static, 0.001, "iceberg_surface_4_static.stim"),
    (6, MeasurementStrategy.Static, 0.001, "iceberg_surface_6_static.stim"),
    (4, MeasurementStrategy.Adaptive, 0.0, "iceberg_surface_4_adaptive.stim"),
    (6, MeasurementStrategy.Adaptive, 0.0, "iceberg_surface_6_adaptive.stim"),
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


@pytest.mark.parametrize(("distance", "strategy", "p", "circuit_name"), CSF_CASES)
def test_concatenated_surface_code_get_memory(
    distance: int,
    strategy: MeasurementStrategy,
    p: float,
    circuit_name: str,
) -> None:
    code = ConcatenatedSurfaceCode(distance)
    reference_path = DATA / "circuits" / circuit_name

    circuit, detection_events, observable_flips = code.get_memory(
        rounds=distance, p=p, strategy=strategy
    )

    expected = stim.Circuit.from_file(str(reference_path))

    assert circuit == expected

    assert isinstance(detection_events, np.ndarray)
    assert isinstance(observable_flips, np.ndarray)

    assert detection_events.dtype == bool
    assert observable_flips.dtype == bool
    
    assert detection_events.shape == (circuit.num_detectors,)
    assert observable_flips.shape == (circuit.num_observables,)
