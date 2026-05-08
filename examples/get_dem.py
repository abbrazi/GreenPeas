#!/usr/bin/env python3

from greenpeas.codes import SurfaceCode
from greenpeas.error_analysis import CorrelationLevel, get_driver

code = SurfaceCode(3)
circuit = code.get_memory(rounds=3, p=0.001)
driver = get_driver(circuit, correlation_level=CorrelationLevel.L0)
dem = driver.compile(circuit)

print(dem)
