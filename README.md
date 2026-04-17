# O.T.I.S. — Optical Threat Isolation System

![Build otis-core](https://github.com/SkyGuardDefense/otis-core/actions/workflows/build.yml/badge.svg)

**SkyGuard Defense Systems / InSitu Labs — Denver, CO**
*otis-core v0.1 — April 2026*

O.T.I.S. is a geometry-first C-UAS computer vision pipeline that defeats adversarial optical spoofing — specifically the FlyTrap Distance-Pulling Attack (DPA) published at NDSS 2026 by UC Irvine researchers.

Rather than improving optical anomaly detection, O.T.I.S. exits that arms race by tracking physical mass, momentum, and volumetric continuity instead of camera-derived pixel data.

---

## Four-Stage Pipeline

| Stage | Name | Function | Status |
|---|---|---|---|
| 1 | Spectral Delamination Matrix (SDM) | Per-channel spectral filtering; adversarial pattern suppression | Working (Python + WebGL) |
| 2 | Kinetic Contour Mapping (KCM) | Phase-congruent edge detection; illumination-invariant structural boundaries | Working (C++ + GLSL) |
| 3 | Volumetric Stereo-Triangulation (VST) | Live stereo depth mesh; real geometry vs. monocular inference | PoC (Python/OpenCV) |
| 4 | Structural Skeletonization | 3D point-cloud wireframe; center-of-mass lock; intercept vector | Specified — forward milestone |

---

## Repository Structure
