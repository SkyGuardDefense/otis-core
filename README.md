<p align="center">
  <img src="https://skyguarddefense.com/skyguard_logo.png" alt="SkyGuard Defense Systems" width="200"/>
</p>

# otis-core

![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)
![Language](https://img.shields.io/badge/Language-C++17-orange.svg)
![Build](https://img.shields.io/badge/Build-CMake-brightgreen.svg)

**otis-core** is a bare-metal mathematical isolation engine designed to mitigate zero-day vulnerabilities in open-source defense robotics. Maintained by **SkyGuard Defense Systems**, this library bypasses highly exploitable general-purpose operating systems and video codecs (e.g., Ubuntu, ROS 2, FFmpeg). By replacing vulnerable video buffers with Kinetic Contour Mapping (KCM), `otis-core` prevents AI-generated Remote Code Execution (RCE) attacks during kinetic intercepts.

---

## 1. The Open-Source Zero-Day Crisis

The global defense robotics industry relies heavily on the Open Platform Targeting & Intercept Control Stack (O.P.T.I.C.S.). Recent threat models, including autonomous AI vulnerability discovery (e.g., Anthropic's Mythos model), have proven that relying on standard video buffers and middleware is a terminal liability.

Adversaries do not need to physically spoof a drone. A single maliciously encoded video frame can trigger a buffer overflow in standard open-source video codecs (like FFmpeg or GStreamer), granting Remote Code Execution (RCE) and Root access before the targeting matrix even processes the image. 

`otis-core` was built specifically to solve this vulnerability. By physically and mathematically air-gapping the targeting logic from the video codec layer, we discard the OS dependency and eliminate the attack vector entirely.

---

## 2. The Solution: Kinetic Contour Mapping (KCM)

Rather than parsing manipulatable pixels through exploitable software pipelines, `otis-core` forces target lock through undeniably continuous physical mass and velocity. The core engine is built around the following objective function:

$$\text{minimize } J(\mathbf{r}_{CG}) = \int \left[ \alpha \left\| \sum_i (\rho_i \mathbf{v}_i \delta V) - P_{predict} \right\|^2 + \beta(\nabla V) + \gamma(Feature\_Override) \right] dt$$

### Mathematical Breakdown
* **Zero-Day Bypass / Mass Tracking:** $\left\| \sum_i (\rho_i \mathbf{v}_i \delta V) - P_{predict} \right\|^2$ calculates the actual physical mass and momentum vectors of the target. By isolating this data at the hardware level, it neutralizes software-based codec exploits.
* **Spoofing Penalty:** $\beta(\nabla V)$ applies a steep gradient penalty for rapid, physically impossible volume changes, instantly neutralizing morphological topological spoofing as a secondary defense.
* **Feature Discard:** $\gamma(Feature\_Override)$ explicitly discards traditional, exploitable feature-matching matrices that rely on pixel contrast.

Mathematical truth defeats software exploits.

---

## 3. Repository Architecture

The codebase is structured to compile efficiently on high-performance edge computing devices (e.g., NVIDIA Jetson, custom FPGAs) without relying on heavy OS-level dependencies.

* `/include/otis_core/`: Core headers defining the KCM math models, 3D vectors, and bare-metal memory structs.
* `/src/`: C++ implementation of the geometric engine and matrix calculations.
* `/src/bypasses/`: Contains the critical `ffmpeg_bypass_layer.cpp` logic demonstrating how sensor photon data is routed directly to kinematics, skipping standard codecs entirely.
* `/examples/`: Mock sensor feeds demonstrating how to pipe drone telemetry and raw vision inputs into the KCM matrix to calculate the center of gravity.

---

## 4. Build Instructions

`otis-core` requires **CMake (>= 3.10)** and a **C++17** compatible compiler. It is designed to be highly modular and free of external bloat.

### Linux / macOS
```bash
git clone [https://github.com/skyguarddefense/otis-core.git](https://github.com/skyguarddefense/otis-core.git)
cd otis-core
mkdir build && cd build
cmake ..
make
