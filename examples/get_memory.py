#!/usr/bin/env python3

from greenpeas.codes import SurfaceCode

code = SurfaceCode(3)

circuit = code.get_memory(rounds=3, p=0.001)

print(circuit)
