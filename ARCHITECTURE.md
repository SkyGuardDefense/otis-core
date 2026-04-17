# ARCHITECTURE.md — O.T.I.S. Pipeline Status
*SkyGuard Defense Systems — otis-core v0.1*

This document is an honest, current-state accounting of the O.T.I.S. pipeline.
Engineers deserve accurate signal.

---

## Pipeline Overview

| Stage | Name | Function | Status |
|---|---|---|---|
| 1 | Spectral Delamination Matrix (SDM) | Independent per-channel spectral filtering | Working (Python + WebGL) |
| 2 | Kinetic Contour Mapping (KCM) | Phase-congruent edge detection | Working (C++ + GLSL) |
| 3 | Volumetric Stereo-Triangulation (VST) | Live stereo depth mesh | PoC (Python/OpenCV) |
| 4 | Structural Skeletonization | Center-of-mass lock; intercept vector | Specified — forward milestone |

---

## Current Code Inventory

### C++ Core
- `include/otis_core/kcm_math.hpp` — KCMEngine class, Vector3D, TargetMass structs
- `include/otis_core/otis_core.hpp` — KCM_Engine class with alpha/beta/gamma weights
- `src/kinematics_math.cpp` — KCM objective function, momentum tracking, volume penalty
- `src/bypasses/ffmpeg_bypass_layer.cpp` — Codec bypass interface (API specified)
- `examples/mock_sensor_feed.cpp` — FlyTrap spoofing simulation (volume_delta=0.45)
- `examples/mock_telemetry_feed.cpp` — Multi-frame inertial CG tracking demo

### Python Validation
- `python-validation/kcm_validate.py` — Phase-edge analysis on FlyTrap test images
- `python-validation/stereo_disparity_poc.py` — OpenCV StereoSGBM depth map
- `python-validation/volume_anomaly_detector.py` — Frame-by-frame beta(nablaV) scoring

### WebGL Demo
- `web-demo/kcm_visualizer.html` — Live browser demo, mobile GPU accelerated
- `web-demo/shaders/kcm_fragment.glsl` — Phase-edge GLSL shader (TopoLight derivative)
- `web-demo/shaders/volume_penalty.glsl` — Volumetric boundary anomaly detection
- `web-demo/js/kcm_engine.js` — JS wrapper connecting shader output to KCM scoring

---

## What This Is NOT (Yet)

- Not a fielded or field-tested system
- Hardware stereo rig not acquired or tested
- Density estimation uses uniform rho=1.0
- theta_morph threshold is geometrically reasoned, not empirically calibrated
- ROS 2 integration is specified, not implemented
- No live hardware intercept has been performed

---

## Forward Milestones

| Milestone | Dependency | Complexity |
|---|---|---|
| Non-uniform density estimation | IR or radar sensor integration | High |
| theta_morph empirical calibration | Physical stereo rig + FlyTrap test target | Medium |
| ROS 2 sensor bridge | Hardware platform selection | High |
| Full phase congruency (log-Gabor) | GPU compute upgrade | Medium |
| Skeletonization module | VST hardware validation | High |
| Live kinetic intercept test | All above | Very High |

---

## Technology Provenance

Phase-congruency approach developed independently through archaeological specimen CV
(decorrelation stretch + illumination-invariant edge detection, 2024–2025).
C-UAS application identified early 2026 upon review of FlyTrap NDSS 2026 publication.

Prior art: github.com/Xtact/decorrstretch (July 2025 commit)

---
*© 2026 SkyGuard Defense Systems. GPL-3.0 and SkyGuard Commercial License.*
