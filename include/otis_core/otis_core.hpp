/*
 * Copyright (c) 2026 Colby Fitch / SkyGuard Defense Systems.
 * Released under GPLv3. Commercial licenses available.
 */

#pragma once

#include <vector>
#include <cmath>
#include <iostream>

namespace skyguard {
namespace otis {

    // 3D Vector representation for physics calculations
    struct Vector3D {
        double x, y, z;
        double norm() const { return std::sqrt(x*x + y*y + z*z); }
    };

    // Sensor state packet bypassing standard video decoders
    struct SensorData {
        double timestamp;
        std::vector<double> density_matrix;
        std::vector<Vector3D> velocity_vectors;
        double volume_delta;
    };

    class KCM_Engine {
    public:
        KCM_Engine(double alpha, double beta, double gamma);
        
        // Main Objective Function: minimize J(r_CG)
        double calculate_rcg(const SensorData& data, const Vector3D& p_predict);

    private:
        double alpha_weight;
        double beta_weight;
        double gamma_weight;

        double process_momentum_vectors(const SensorData& data, const Vector3D& p_predict);
        double apply_volume_penalty(double volume_delta);
        void discard_optical_features();
    };

} // namespace otis
} // namespace skyguard
