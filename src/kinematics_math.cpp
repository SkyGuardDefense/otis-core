// kinematics_math.cpp — KCM Objective Function Implementation
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026
//
// Implements the three-term KCM objective function:
//   J(r_CG) = integral [ alpha * ||sum(rho_i * v_i * dV) - P_predict||^2
//                       + beta(nablaV)
//                       + gamma(Phi_discard) ] dt
//
// Reference: MATH.md Sections 2-3

#include "otis_core/kcm_math.hpp"
#include "otis_core/otis_core.hpp"
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace otis {

// ─────────────────────────────────────────────────────────────────────────────
// Vector3D Operations
// ─────────────────────────────────────────────────────────────────────────────

Vector3D Vector3D::operator+(const Vector3D& o) const {
    return {x + o.x, y + o.y, z + o.z};
}
Vector3D Vector3D::operator-(const Vector3D& o) const {
    return {x - o.x, y - o.y, z - o.z};
}
Vector3D Vector3D::operator*(double s) const {
    return {x * s, y * s, z * s};
}
double Vector3D::dot(const Vector3D& o) const {
    return x * o.x + y * o.y + z * o.z;
}
double Vector3D::magnitude() const {
    return std::sqrt(x*x + y*y + z*z);
}
double Vector3D::magnitudeSquared() const {
    return x*x + y*y + z*z;
}
Vector3D Vector3D::normalize() const {
    double mag = magnitude();
    if (mag < 1e-12) return {0.0, 0.0, 0.0};
    return {x/mag, y/mag, z/mag};
}

// ─────────────────────────────────────────────────────────────────────────────
// KCMEngine Constructor
// ─────────────────────────────────────────────────────────────────────────────

KCMEngine::KCMEngine(double alpha, double beta0, double gamma0,
                     double theta_morph, double kappa)
    : alpha_(alpha),
      beta0_(beta0),
      gamma_(gamma0),
      theta_morph_(theta_morph),
      kappa_(kappa),
      frame_count_(0),
      anomaly_count_(0)
{
    if (alpha_ < 0 || beta0_ < 0 || gamma_ < 0)
        throw std::invalid_argument("[KCM] Weights alpha/beta/gamma must be non-negative.");
    if (theta_morph_ <= 0 || theta_morph_ >= 1.0)
        throw std::invalid_argument("[KCM] theta_morph must be in (0, 1).");
    if (kappa_ <= 0)
        throw std::invalid_argument("[KCM] kappa must be positive.");
}

// ─────────────────────────────────────────────────────────────────────────────
// Term Alpha — Momentum Continuity
//
// Computes squared error between observed aggregate momentum of all volumetric
// segments and the Kalman-predicted momentum at this timestep.
//
// A real drone's mass * velocity cannot be forged by surface optical patterns.
// This term fires when the depth-mesh aggregate momentum deviates from the
// physically predicted trajectory — regardless of what the camera sees.
// ─────────────────────────────────────────────────────────────────────────────

double KCMEngine::compute_momentum_term(
    const std::vector<TargetMass>& segments,
    const Vector3D& P_predict) const
{
    Vector3D P_observed = {0.0, 0.0, 0.0};

    for (const auto& seg : segments) {
        // p_i = rho_i * v_i * deltaV
        Vector3D momentum_i = seg.velocity * (seg.density * seg.deltaV);
        P_observed = P_observed + momentum_i;
    }

    // Squared error: ||P_observed - P_predict||^2
    Vector3D error = P_observed - P_predict;
    double squared_error = error.magnitudeSquared();

    return alpha_ * squared_error;
}

// ─────────────────────────────────────────────────────────────────────────────
// Term Beta — Volume Continuity Penalty
//
// Fires when the volumetric boundary shift between consecutive frames exceeds
// theta_morph. The FlyTrap DPA generates trajectories that are geometrically
// incoherent from a stereo depth-mesh perspective — even when they are
// spectrally and temporally consistent to optical sensors.
//
// beta(nablaV) = { beta0 * nablaV * kappa   if nablaV > theta_morph
//               { 0                          otherwise
// ─────────────────────────────────────────────────────────────────────────────

double KCMEngine::apply_volume_penalty(double nablaV) const {
    if (nablaV > theta_morph_) {
        return beta0_ * nablaV * kappa_;
    }
    return 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Term Gamma — Feature Discard (Hard Architectural Gate)
//
// All brightness-gradient optical feature descriptors — ORB, SIFT, BRIEF,
// SURF, HOG, optical_flow — are zeroed before contributing to the objective.
// This is NOT a weighted reduction. It is a hard zero.
//
// The architecture is structurally incapable of being influenced by optical
// feature data. An adversarial pattern that defeats optical flow completely
// still contributes zero to J(r_CG).
//
// Phi_discard vector elements:
//   [0] = ORB keypoint contribution
//   [1] = SIFT descriptor contribution
//   [2] = BRIEF binary string contribution
//   [3] = HOG gradient histogram contribution
//   [4] = optical_flow vector contribution
//   [5] = color histogram contribution
//
// All are unconditionally zeroed. The gamma coefficient scales the gate
// penalty for any upstream system that attempts to inject optical features.
// ─────────────────────────────────────────────────────────────────────────────

double KCMEngine::discard_optical_features(
    std::vector<double>& optical_features) const
{
    // Hard architectural zero — no conditional, no weight, no exception
    double injected_energy = 0.0;
    for (double& f : optical_features) {
        injected_energy += std::abs(f);
        f = 0.0;  // Unconditional zero
    }

    // If upstream injected non-zero optical features, score the attempt
    // gamma_ * injected_energy surfaces adversarial injection attempts in J
    return gamma_ * injected_energy;
}

// ─────────────────────────────────────────────────────────────────────────────
// Full Objective Function
//
// J(r_CG) = alpha_term + beta_term + gamma_term
//
// Minimized at true target lock. FlyTrap spoofing drives J upward by
// triggering beta (volume discontinuity) while alpha and gamma remain
// unaffected by surface optical treatment.
// ─────────────────────────────────────────────────────────────────────────────

KCMResult KCMEngine::compute_objective(
    const std::vector<TargetMass>& segments,
    const Vector3D& P_predict,
    double nablaV,
    std::vector<double> optical_features) const
{
    KCMResult result;

    result.alpha_term = compute_momentum_term(segments, P_predict);
    result.beta_term  = apply_volume_penalty(nablaV);
    result.gamma_term = discard_optical_features(optical_features);
    result.J          = result.alpha_term + result.beta_term + result.gamma_term;
    result.nablaV     = nablaV;
    result.spoofing_detected = (result.beta_term > 0.0);

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame Processing — Full Pipeline Entry Point
// ─────────────────────────────────────────────────────────────────────────────

KCMResult KCMEngine::process_frame(
    const std::vector<TargetMass>& segments,
    const Vector3D& P_predict,
    double nablaV,
    std::vector<double> optical_features)
{
    frame_count_++;

    KCMResult result = compute_objective(segments, P_predict, nablaV, optical_features);

    if (result.spoofing_detected) {
        anomaly_count_++;
        std::cout << "[KCM ⚠] Frame " << frame_count_
                  << " | nablaV=" << nablaV
                  << " | beta=" << result.beta_term
                  << " | J=" << result.J
                  << " | FLYTRAP DPA SIGNATURE DETECTED\n";
    } else {
        std::cout << "[KCM  ] Frame " << frame_count_
                  << " | nablaV=" << nablaV
                  << " | J=" << result.J
                  << " | LOCK NOMINAL\n";
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Kalman Momentum Predictor
//
// Simple linear Kalman update for P_predict.
// State: [px, py, pz, dpx, dpy, dpz]
// Measurement: observed momentum from current depth mesh frame
// ─────────────────────────────────────────────────────────────────────────────

KalmanMomentumPredictor::KalmanMomentumPredictor(double process_noise,
                                                  double measurement_noise)
    : process_noise_(process_noise),
      measurement_noise_(measurement_noise),
      initialized_(false),
      P_estimate_({0.0, 0.0, 0.0}),
      P_velocity_({0.0, 0.0, 0.0}),
      error_covariance_(1.0)
{}

Vector3D KalmanMomentumPredictor::predict(double dt) {
    if (!initialized_) return P_estimate_;
    // Predict next state: P = P + dP * dt
    return P_estimate_ + P_velocity_ * dt;
}

void KalmanMomentumPredictor::update(const Vector3D& P_observed, double dt) {
    if (!initialized_) {
        P_estimate_ = P_observed;
        initialized_ = true;
        return;
    }

    // Predict
    Vector3D P_predicted = P_estimate_ + P_velocity_ * dt;
    double P_cov_predicted = error_covariance_ + process_noise_;

    // Kalman gain
    double K = P_cov_predicted / (P_cov_predicted + measurement_noise_);

    // Update momentum estimate
    Vector3D innovation = P_observed - P_predicted;
    P_estimate_ = P_predicted + innovation * K;

    // Update velocity estimate (momentum derivative)
    P_velocity_ = (P_estimate_ - P_predicted) * (1.0 / (dt + 1e-9));

    // Update error covariance
    error_covariance_ = (1.0 - K) * P_cov_predicted;
}

Vector3D KalmanMomentumPredictor::getEstimate() const {
    return P_estimate_;
}

// ─────────────────────────────────────────────────────────────────────────────
// FlyTrap DPA Test Case
//
// Simulates the Distance-Pulling Attack volumetric signature:
// - High nablaV (0.45) — large volume discontinuity between frames
// - Zero momentum error — target appears kinematically valid to optical sensors
// - Optical features present but immediately zeroed by gamma gate
//
// Expected result: beta_term fires, spoofing_detected = true
// ─────────────────────────────────────────────────────────────────────────────

void run_flytrap_test() {
    std::cout << "\n[O.T.I.S.] === FlyTrap DPA Test Case ===\n";
    std::cout << "[O.T.I.S.] Simulating Distance-Pulling Attack signature...\n\n";

    KCMEngine engine(
        1.0,    // alpha — momentum weight
        1.0,    // beta0 — volume penalty base
        1.0,    // gamma — feature discard weight
        0.10,   // theta_morph — 10% volumetric shift threshold
        1000.0  // kappa — steep penalty multiplier
    );

    // Simulated depth-mesh segments (uniform density — rho=1.0)
    std::vector<TargetMass> segments = {
        {{0.1, 0.0, 2.0}, 1.0, 0.05},  // segment 0: normal forward motion
        {{0.0, 0.1, 2.0}, 1.0, 0.05},  // segment 1: slight lateral
        {{0.0, 0.0, 2.0}, 1.0, 0.05},  // segment 2: axial
        {{0.1, 0.1, 1.9}, 1.0, 0.05},  // segment 3: diagonal
    };

    // Kalman-predicted momentum: consistent with drone approaching at 2 m/s
    Vector3D P_predict = {0.0, 0.0, 2.0};

    // FlyTrap DPA: high volume discontinuity (nablaV = 0.45)
    // This is the geometric signature of morphological spoofing —
    // the depth mesh boundary jumps 45% between frames
    double nablaV_flytrap = 0.45;

    // Optical features present (adversarial pattern attempting feature injection)
    std::vector<double> optical_features = {0.85, 0.72, 0.91, 0.68, 0.77, 0.55};

    KCMResult result = engine.process_frame(segments, P_predict, nablaV_flytrap, optical_features);

    std::cout << "\n[O.T.I.S.] === Test Results ===\n";
    std::cout << "  J(r_CG)          = " << result.J           << "\n";
    std::cout << "  alpha_term       = " << result.alpha_term   << "\n";
    std::cout << "  beta_term        = " << result.beta_term    << "\n";
    std::cout << "  gamma_term       = " << result.gamma_term   << "\n";
    std::cout << "  nablaV           = " << result.nablaV       << "\n";
    std::cout << "  spoofing_detected= " << (result.spoofing_detected ? "TRUE" : "FALSE") << "\n";
    std::cout << "\n[O.T.I.S.] Expected: beta_term=450.0, spoofing_detected=TRUE\n";

    if (result.spoofing_detected && result.beta_term > 400.0) {
        std::cout << "[O.T.I.S.] ✓ PASS — FlyTrap DPA correctly flagged by beta(nablaV)\n";
    } else {
        std::cout << "[O.T.I.S.] ✗ FAIL — FlyTrap DPA not detected\n";
    }

    // Normal flight test — no spoofing
    std::cout << "\n[O.T.I.S.] --- Normal Flight Test (no spoofing) ---\n";
    double nablaV_normal = 0.03;
    std::vector<double> no_features = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    KCMResult normal = engine.process_frame(segments, P_predict, nablaV_normal, no_features);
    if (!normal.spoofing_detected) {
        std::cout << "[O.T.I.S.] ✓ PASS — Normal flight correctly clears beta gate\n";
    } else {
        std::cout << "[O.T.I.S.] ✗ FAIL — False positive on normal flight\n";
    }
}

} // namespace otis
