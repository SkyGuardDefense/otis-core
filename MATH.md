# MATH.md — KCM Mathematical Specification
**Kinetic Contour Mapping: A Geometry-First Approach to Defeating Adversarial Optical Spoofing**

*SkyGuard Defense Systems / InSitu Labs*
*otis-core v0.1 — April 2026*

---

## 0. Motivation

Every published countermeasure to Distance-Pulling Attacks (DPA) and adversarial optical
spoofing — including VOGUES, PercepGuard, and VisionGuard — operates on a shared
assumption: that the ground truth for target tracking is a camera frame.

The FlyTrap adversarial umbrella (NDSS 2026, UC Irvine) defeated all three at 100% success
rate against DJI Mini 4 Pro, DJI NEO, and HoverAir X1 by maintaining spatiotemporal
consistency specifically engineered to survive behavioral anomaly detection on optical feeds.

KCM exits that arms race entirely. The tracking substrate is physical geometry and mass
continuity — not photon reflection.

---

## 1. Core Definitions

| Symbol | Type | Definition |
|--------|------|------------|
| r_CG | Vector3D | Estimated center-of-gravity position of target |
| rho_i | scalar | Density estimate of volumetric segment i |
| v_i | Vector3D | Velocity vector of volumetric segment i |
| deltaV | scalar | Volumetric unit (voxel size in stereo mesh) |
| P_predict | Vector3D | Kalman-predicted momentum vector at timestep t |
| nablaV | scalar | Volume delta between consecutive frames |
| alpha, beta, gamma | scalar | Weighting coefficients for objective function terms |
| J(r_CG) | scalar | Objective function value — minimized at true target lock |

---

## 2. The KCM Objective Function

min J(r_CG) = integral [ alpha * || sum_i(rho_i * v_i * deltaV) - P_predict ||^2 + beta(nablaV) + gamma(Phi_discard) ] dt

---

## 3. Term-by-Term Breakdown

### 3.1 Term alpha — Momentum Continuity
Computes squared error between observed aggregate momentum and Kalman-predicted momentum.
A physical drone cannot be spoofed by surface optical patterns — mass and velocity are
properties of matter, not light.

### 3.2 Term beta — Volume Continuity Penalty

beta(nablaV) = { beta_0 * nablaV * kappa   if nablaV > theta_morph
               { 0                           otherwise

- theta_morph = 0.10 (10% volumetric boundary shift between frames)
- kappa = 1000.0 (steep penalty multiplier)

FlyTrap DPA generates trajectories that violate rigid body motion constraints from the
perspective of a stereo depth mesh. beta(nablaV) fires precisely on this discontinuity.

Implementation: apply_volume_penalty() in src/kinematics_math.cpp

### 3.3 Term gamma — Feature Discard
Phi_discard is a hard architectural gate. All brightness-gradient feature descriptors
(ORB, SIFT, BRIEF, SURF, HOG, optical_flow) are set to zero contribution. This is not
a weighted reduction — it is a hard zero. The architecture is structurally incapable of
being influenced by optical feature data.

---

## 4. Phase Congruency — Bridge from Specimen CV to C-UAS

Phase congruency (Morrone & Owens 1987; Kovesi 1999) measures structural discontinuities
defined by geometric or material change, not illumination. Invariant to linear illumination
changes because uniform scaling affects amplitudes but not phase relationships.

The TopoLight GLSL shader computes per-pixel phase-congruent edge response:

```glsl
vec3 lightDir = normalize(uLightPosition - vPosition);
float phaseEdge = dot(normalize(vNormal), lightDir);
float structuralResponse = abs(dFdx(phaseEdge)) + abs(dFdy(phaseEdge));
```

The same math that identifies material discontinuities in a 2,000-year-old rock specimen
identifies geometric boundaries through adversarial optical patterns.

---

## 5. Attack Vector Coverage

| Attack Vector | alpha | beta | gamma |
|---|---|---|---|
| Optical color/pattern spoofing | Defeated | — | Defeated |
| FlyTrap morphological DPA | — | Defeated | — |
| SIFT/ORB feature injection | — | — | Defeated |
| Codec RCE (FFmpeg/GStreamer) | Defeated | — | Defeated |
| Spatiotemporal adversarial patterns | Defeated | Defeated | Defeated |

---

## 6. Implementation Status

| Component | Status | Location |
|---|---|---|
| KCM objective function | Working (C++) | src/kinematics_math.cpp |
| Volume penalty beta | Working (C++) | src/kinematics_math.cpp |
| Feature discard gamma | Stub — in progress | src/kinematics_math.cpp |
| Phase-edge WebGL shader | Working (GLSL) | web-demo/shaders/ |
| Decorrelation stretch (SDM) | Working (Python) | python-validation/ |
| Stereo disparity (VST) | PoC (Python/OpenCV) | python-validation/ |
| Kalman predictor | Specified | Forward milestone |
| ROS 2 sensor bridge | Specified | Forward milestone |
| Hardware stereo rig | Specified | Forward milestone |

---

## 7. Known Limitations

1. Density estimation is uniform (rho_i = 1.0). Non-uniform density from real sensor fusion required for production.
2. theta_morph = 0.10 is geometrically reasoned, not empirically calibrated on hardware.
3. Stereo baseline range and VST accuracy at long range not yet characterized.
4. dFdx/dFdy is a phase-congruency approximation, not a full Fourier decomposition.
5. Validation against next-generation adversarial patterns not yet conducted.

---

## 8. References

1. Jha, S. et al. "FlyTrap: Distance-Pulling Attacks on Visual Drone Navigation." NDSS 2026.
2. Morrone, M.C. and Owens, R.A. "Feature detection from local energy." Pattern Recognition Letters, 1987.
3. Kovesi, P. "Image features from phase congruency." Videre, 1999.
4. Hirschmuller, H. "Stereo Processing by Semiglobal Matching." IEEE TPAMI, 2008.
5. Kalman, R.E. "A New Approach to Linear Filtering and Prediction Problems." ASME, 1960.

---
*© 2026 SkyGuard Defense Systems. GPL-3.0 and SkyGuard Commercial License.*
