import numpy as np
import pylib

def test_foo():
    assert(pylib.foo(40,2) == 42)

def test_sum():
    a = np.arange(10, dtype=np.float32)
    res = pylib.sum(a)
    assert(res == 45.0)
