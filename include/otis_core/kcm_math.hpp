#pragma once

#include <vector>
#include <cmath>
#include <array>

namespace otis {
namespace math {

    // 3D Vector for bare-metal kinematics
    struct Vector3D {
        double x, y, z;
        
        Vector3D operator-(const Vector3D& other) const {
            return {x - other.x, y - other.y, z - other.z};
        }
        
        double magnitude_squared() const {
            return (x*x) + (y*y) + (z*z);
        }
    };

    // Kinetic Contour Mapping (KCM) Target Matrix
    struct TargetMass {
        Vector3D position;
        Vector3D velocity;
        double density_rho;
        double volume_delta;
    };

    class KCMEngine {
    private:
        // Core tuning parameters for the optimization integral
        static constexpr double ALPHA = 1.0; // Momentum weight
        static constexpr double BETA = 0.8;  // Spoofing penalty weight
        
        // Discard standard feature extraction entirely (Zero-Day bypass)
        static constexpr bool FEATURE_OVERRIDE = true; 

    public:
        KCMEngine() = default;

        /**
         * @brief Calculates the inertial center of gravity, ignoring topological visual data.
         * J(r_CG) = ∫ [ α || Σ(ρ_i v_i δV) - P_predict ||² + β(∇V) ]
         */
        Vector3D calculate_inertial_cg(const std::vector<TargetMass>& contour_data, const Vector3D& predicted_pos) {
            Vector3D sum_momentum = {0.0, 0.0, 0.0};
            double total_mass = 0.0;
            double volume_gradient_penalty = 0.0;

            for (size_t i = 0; i < contour_data.size(); ++i) {
                const auto& point = contour_data[i];
                
                // Calculate physical mass matrix (ρ * δV)
                double localized_mass = point.density_rho * point.volume_delta;
                
                sum_momentum.x += localized_mass * point.velocity.x;
                sum_momentum.y += localized_mass * point.velocity.y;
                sum_momentum.z += localized_mass * point.velocity.z;

                total_mass += localized_mass;

                // Apply morphological spoofing penalty (β(∇V))
                if (i > 0) {
                    double vol_change = std::abs(point.volume_delta - contour_data[i-1].volume_delta);
                    if (vol_change > 0.5) { // Rapid unnatural volume shift detected
                        volume_gradient_penalty += (vol_change * BETA);
                    }
                }
            }

            // If feature override is true, we strictly use the kinetic matrix
            if (total_mass > 0 && FEATURE_OVERRIDE) {
                Vector3D isolated_cg = {
                    (sum_momentum.x / total_mass) - predicted_pos.x,
                    (sum_momentum.y / total_mass) - predicted_pos.y,
                    (sum_momentum.z / total_mass) - predicted_pos.z
                };
                return isolated_cg;
            }

            return {0.0, 0.0, 0.0}; // Fallback
        }
    };

} // namespace math
} // namespace otis
