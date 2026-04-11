#include <iostream>
#include <vector>
#include "../include/otis_core/kcm_math.hpp"

using namespace otis::math;

int main() {
    std::cout << "[O.T.I.S. CORE] Initializing Kinetic Contour Mapping (KCM) Engine...\n";
    KCMEngine engine;

    // Simulated raw photon / hardware telemetry (Bypassing FFmpeg buffers)
    // Format: Position(x,y,z), Velocity(x,y,z), Density, Volume_Delta
    std::vector<TargetMass> mock_target_data = {
        {{100.0, 50.0, 200.0}, {15.0, -2.0, 0.0}, 1.2, 0.5},
        {{101.0, 51.0, 200.5}, {15.0, -2.1, 0.0}, 1.2, 0.51},
        {{99.0, 49.0, 199.5}, {14.9, -1.9, 0.0}, 1.2, 0.49}
    };

    Vector3D predicted_trajectory = {115.0, 48.0, 200.0};

    std::cout << "[SYS] Telemetry acquired. Calculating Inertial CG...\n";
    
    Vector3D calculated_cg = engine.calculate_inertial_cg(mock_target_data, predicted_trajectory);

    std::cout << "[SUCCESS] Target Lock Achieved.\n";
    std::cout << " -> Isolated CG Offset: X: " << calculated_cg.x 
              << " | Y: " << calculated_cg.y 
              << " | Z: " << calculated_cg.z << "\n";
    std::cout << "[SECURE] Video codec dependencies bypassed.\n";

    return 0;
}
