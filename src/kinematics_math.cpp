#include "otis_core/otis_core.hpp"

namespace skyguard {
namespace otis {

    KCM_Engine::KCM_Engine(double alpha, double beta, double gamma) 
        : alpha_weight(alpha), beta_weight(beta), gamma_weight(gamma) {}

    double KCM_Engine::calculate_rcg(const SensorData& data, const Vector3D& p_predict) {
        // Step 1: Discard exploitable optical feature sets
        discard_optical_features();

        // Step 2: Calculate inertial mass tracking 
        double momentum_val = process_momentum_vectors(data, p_predict);

        // Step 3: Apply gradient penalty for impossible volume shifts
        double volume_penalty = apply_volume_penalty(data.volume_delta);

        // Result of the integral J(r_CG)
        return (alpha_weight * momentum_val) + (beta_weight * volume_penalty);
    }

    double KCM_Engine::process_momentum_vectors(const SensorData& data, const Vector3D& p_predict) {
        double total_momentum = 0.0;
        // Sum(rho_i * v_i * deltaV) - P_predict
        for (size_t i = 0; i < data.velocity_vectors.size(); ++i) {
            double rho = data.density_matrix.empty() ? 1.0 : data.density_matrix[i];
            total_momentum += rho * data.velocity_vectors[i].norm();
        }
        double error = total_momentum - p_predict.norm();
        return error * error; // Squared norm error
    }

    double KCM_Engine::apply_volume_penalty(double volume_delta) {
        // β(∇V) - Penalizes morphological spoofing instantly
        if (volume_delta > 0.1) {
            return volume_delta * 1000.0; // High penalty for topological shifting
        }
        return 0.0;
    }

    void KCM_Engine::discard_optical_features() {
        // Explicitly bypasses feature-matching matrices (e.g., ORB, SIFT)
        // System relies strictly on geometry and mass.
    }

} // namespace otis
} // namespace skyguard
