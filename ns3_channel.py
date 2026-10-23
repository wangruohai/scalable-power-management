"""
Load libpm-ns3-gains.so and call ns3_channel_gains_scm.
"""

import ctypes
from pathlib import Path

import numpy as np

SO = Path("/home/tan/ns-3-nr/build/lib/libpm-ns3-gains.so")
_LIB = None
_FORCED_STREAM = None
_STREAM_CTR = 1


def use_spatial(on=True):
    """Experiment scripts call this. This build only provides the spatial channel."""
    if not on:
        raise RuntimeError("this build only provides ns3_channel_gains_scm")


def set_stream(stream):
    """Fix the next C-API stream, or None to auto-increment."""
    global _FORCED_STREAM
    _FORCED_STREAM = None if stream is None else int(stream)


def _take_stream(n_pairs=8):
    global _STREAM_CTR
    if _FORCED_STREAM is not None:
        return _FORCED_STREAM
    s = _STREAM_CTR
    _STREAM_CTR += max(int(n_pairs), 1)
    return s


def _lib():
    global _LIB
    if _LIB is None:
        _LIB = ctypes.CDLL(str(SO))
        args = [
            ctypes.c_int, ctypes.c_int,
            ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.c_double), ctypes.POINTER(ctypes.c_double),
            ctypes.POINTER(ctypes.c_double),
            ctypes.c_int64,
        ]
        _LIB.ns3_channel_gains_scm.argtypes = args
        _LIB.ns3_channel_gains_scm.restype = ctypes.c_int
    return _LIB


def _invoke(n_u, n_a, ux, uy, ax, ay, tilt, az, H, stream):
    lib = _lib()
    uxp = ux.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    uyp = uy.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    axp = ax.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    ayp = ay.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    tp = tilt.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    zp = az.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    hp = H.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    rc = lib.ns3_channel_gains_scm(n_u, n_a, uxp, uyp, axp, ayp, tp, zp, hp,
                                   ctypes.c_int64(stream))
    if rc != 0:
        raise RuntimeError("ns3_channel_gains_scm failed")


def channel_gains(net, user_xy, theta, ant_idx=None):
    user_xy = np.asarray(user_xy, dtype=np.float64)
    if ant_idx is None:
        ant_xy, th = net.ant_xy, np.asarray(theta, dtype=np.float64)
    else:
        ant_idx = np.asarray(ant_idx)
        ant_xy = net.ant_xy[ant_idx]
        th = np.asarray(theta, dtype=np.float64)[ant_idx]
    n_u, n_a = int(user_xy.shape[0]), int(ant_xy.shape[0])
    tilt = np.ascontiguousarray(th[:, 0], dtype=np.float64)
    az = np.ascontiguousarray(th[:, 1], dtype=np.float64)
    ux = np.ascontiguousarray(user_xy[:, 0], dtype=np.float64)
    uy = np.ascontiguousarray(user_xy[:, 1], dtype=np.float64)
    ax = np.ascontiguousarray(ant_xy[:, 0], dtype=np.float64)
    ay = np.ascontiguousarray(ant_xy[:, 1], dtype=np.float64)
    H = np.empty((n_u, n_a), dtype=np.float64)
    _invoke(n_u, n_a, ux, uy, ax, ay, tilt, az, H, _take_stream(n_u * n_a))
    return H


def install():
    import expt_scale_invariance
    import wireless

    wireless.channel_gains = channel_gains
    expt_scale_invariance.channel_gains = channel_gains
