#include "otis_core/otis_core.hpp"
#include <iostream>

using namespace skyguard::otis;

int main() {
    std::cout << "--- SKYGUARD O.T.I.S. KINETIC ENGINE ---" << std::endl;
    std::cout << "Booting KCM Matrix..." << std::endl;

    // Initialize engine with Alpha, Beta, Gamma weights
    KCM_Engine engine(1.0, 0.85, 0.0); 

    // Simulate incoming target (Adversarial Drone)
    SensorData target_data;
    target_data.timestamp = 0.01;
    target_data.velocity_vectors.push_back({15.5, 2.0, -1.0}); // Moving fast
    target_data.density_matrix.push_back(5.5); // Solid mass
    
    // Simulate Flytrap morphological spoofing (rapid shape shift)
    target_data.volume_delta = 0.45; 

    Vector3D predicted_momentum = {15.0, 2.0, -1.0};

    std::cout << "Tracking Target CG..." << std::endl;
    double objective_result = engine.calculate_rcg(target_data, predicted_momentum);

    std::cout << "Target Lock Calculated. J(r_CG) = " << objective_result << std::endl;
    std::cout << "Spoofing penalty applied. Optical illusions discarded." << std::endl;

    return 0;
}
