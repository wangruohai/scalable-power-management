# scalable-power-management

Experiment code for *Scalable Real-Time Power Management for Wireless Communication Networks*.

The numerical experiments generate channel gains with ns-3. This repository currently contains that channel and its Python binding. Further experiment code will be added after review.

## `ns3_channel_gains.cc`

```c
int ns3_channel_gains_scm(
    int nUsers, int nAnts,
    const double* ux, const double* uy,
    const double* ax, const double* ay,
    const double* tilt, const double* az,
    double* H, int64_t stream);
```

The routine keeps one base-station node and one user node. For each user-antenna link it moves those two nodes to the requested coordinates, sets the base-station bearing and downtilt, and draws one 3GPP TR 38.901 urban-macro non-line-of-sight spatial channel. `H` is stored in row-major order, `H[u * nAnts + i]`.

Each entry is the large-scale path gain times the sum of squared small-scale channel coefficients. The path gain comes from the UMa propagation model at 2 GHz with log-normal shadowing disabled. The base station uses a single 3GPP antenna element; the user antenna is isotropic. Base-station and user heights are 25 m and 1.5 m. Angles `tilt` and `az` are in degrees.

`stream` is an ns-3 random-stream index passed to `AssignStreams`. Link `k` in the call uses stream `stream + k`, so the same arguments and the same stream reproduce the same gains. A different stream is a different small-scale draw. The function returns 0 on success and 1 if a size or pointer argument is invalid.

Compile the file inside an ns-3 tree that provides the antenna, propagation, and spectrum modules. The Python binding loads the resulting shared library; that binary is not part of this repository.

## `ns3_channel.py`

This module loads `libpm-ns3-gains.so` with `ctypes` and calls `ns3_channel_gains_scm`.

