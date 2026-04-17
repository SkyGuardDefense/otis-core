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
otis-core/
├── README.md
├── MATH.md
├── ARCHITECTURE.md
├── CMakeLists.txt
├── .github/workflows/build.yml
├── src/
│ ├── kinematics_math.cpp
│ └── bypasses/ffmpeg_bypass_layer.cpp
├── include/otis_core/
│ ├── kcm_math.hpp
│ └── otis_core.hpp
├── examples/
│ ├── mock_sensor_feed.cpp
│ └── mock_telemetry_feed.cpp
├── python-validation/
│ ├── kcm_validate.py
│ ├── stereo_disparity_poc.py
│ ├── volume_anomaly_detector.py
│ └── test_images/
└── web-demo/
├── kcm_visualizer.html
├── shaders/
│ ├── kcm_fragment.glsl
│ └── volume_penalty.glsl
└── js/kcm_engine.js

## Quick Start

```bash
git clone https://github.com/SkyGuardDefense/otis-core.git
cd otis-core
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/examples/mock_sensor_feed
```

---

## Live Demo

> WebGL KCM Visualizer — `web-demo/kcm_visualizer.html` — runs in any browser, mobile GPU accelerated.

---

## License

Dual licensed: GPL-3.0 and SkyGuard Commercial License.
*© 2026 SkyGuard Defense Systems.*
