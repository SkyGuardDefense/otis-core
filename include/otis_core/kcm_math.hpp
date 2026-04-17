// kcm_math.hpp — KCM Core Types and Engine Declaration
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026

#pragma once

#include <vector>
#include <string>

namespace otis {

// ─────────────────────────────────────────────────────────────────────────────
// Vector3D — 3D vector with kinematic operations
// ─────────────────────────────────────────────────────────────────────────────

struct Vector3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vector3D operator+(const Vector3D& o) const;
    Vector3D operator-(const Vector3D& o) const;
    Vector3D operator*(double s) const;
    double   dot(const Vector3D& o) const;
    double   magnitude() const;
    double   magnitudeSquared() const;
    Vector3D normalize() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// TargetMass — One volumetric segment of the stereo depth mesh
//
// Each segment represents a voxel in the reconstructed depth mesh.
// rho_i * v_i * deltaV = momentum contribution of segment i
// ─────────────────────────────────────────────────────────────────────────────

struct TargetMass {
    Vector3D velocity;   // v_i — velocity vector of this segment (m/s)
    double   density;    // rho_i — density estimate (1.0 = uniform, forward milestone)
    double   deltaV;     // volumetric unit — voxel size in stereo mesh (m^3)
};

// ─────────────────────────────────────────────────────────────────────────────
// KCMResult — Output of one full objective function evaluation
// ─────────────────────────────────────────────────────────────────────────────

struct KCMResult {
    double J             = 0.0;    // Total objective function value
    double alpha_term    = 0.0;    // Momentum continuity term
    double beta_term     = 0.0;    // Volume continuity penalty
    double gamma_term    = 0.0;    // Feature discard gate penalty
    double nablaV        = 0.0;    // Volume delta this frame
    bool   spoofing_detected = false; // true if beta_term > 0
};

// ─────────────────────────────────────────────────────────────────────────────
// KCMEngine — Main KCM pipeline engine
// ─────────────────────────────────────────────────────────────────────────────

class KCMEngine {
public:
    // Constructor
    // alpha      — momentum continuity weight
    // beta0      — volume penalty base coefficient
    // gamma0     — feature discard gate weight
    // theta_morph— volumetric shift threshold (default 0.10 = 10%)
    // kappa      — steep penalty multiplier (default 1000.0)
    KCMEngine(double alpha      = 1.0,
              double beta0      = 1.0,
              double gamma0     = 1.0,
              double theta_morph= 0.10,
              double kappa      = 1000.0);

    // Full objective function — call once per frame
    KCMResult compute_objective(
        const std::vector<TargetMass>& segments,
        const Vector3D& P_predict,
        double nablaV,
        std::vector<double> optical_features) const;

    // Frame processing with logging
    KCMResult process_frame(
        const std::vector<TargetMass>& segments,
        const Vector3D& P_predict,
        double nablaV,
        std::vector<double> optical_features);

    // Individual terms (public for unit testing)
    double compute_momentum_term(
        const std::vector<TargetMass>& segments,
        const Vector3D& P_predict) const;

    double apply_volume_penalty(double nablaV) const;

    double discard_optical_features(
        std::vector<double>& optical_features) const;

    // State accessors
    int get_frame_count()   const { return frame_count_; }
    int get_anomaly_count() const { return anomaly_count_; }

private:
    double alpha_;
    double beta0_;
    double gamma_;
    double theta_morph_;
    double kappa_;
    int    frame_count_;
    int    anomaly_count_;
};

// ─────────────────────────────────────────────────────────────────────────────
// KalmanMomentumPredictor — Linear Kalman filter for P_predict
// ─────────────────────────────────────────────────────────────────────────────

class KalmanMomentumPredictor {
public:
    KalmanMomentumPredictor(double process_noise     = 0.01,
                             double measurement_noise = 0.1);

    Vector3D predict(double dt);
    void     update(const Vector3D& P_observed, double dt);
    Vector3D getEstimate() const;

private:
    double   process_noise_;
    double   measurement_noise_;
    bool     initialized_;
    Vector3D P_estimate_;
    Vector3D P_velocity_;
    double   error_covariance_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test Runner
// ─────────────────────────────────────────────────────────────────────────────

void run_flytrap_test();

} // namespace otis
