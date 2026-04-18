// mock_sensor_feed.cpp — FlyTrap DPA Spoofing Simulation
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026

#include "otis_core/kcm_math.hpp"
#include "otis_core/otis_core.hpp"
#include <iostream>
#include <vector>

int main() {
    std::cout << "=================================================\n";
    std::cout << " O.T.I.S. — Mock Sensor Feed (FlyTrap DPA Sim)\n";
    std::cout << " SkyGuard Defense Systems — otis-core v0.1\n";
    std::cout << "=================================================\n\n";

    otis::KCMEngine engine(
        1.0,     // alpha
        1.0,     // beta0
        1.0,     // gamma
        0.10,    // theta_morph
        1000.0   // kappa
    );

    otis::KalmanMomentumPredictor kalman(0.01, 0.1);

    std::vector<otis::TargetMass> segments = {
        {{0.10, 0.00, 2.00}, 1.0, 0.05},
        {{0.00, 0.10, 2.00}, 1.0, 0.05},
        {{0.00, 0.00, 2.00}, 1.0, 0.05},
        {{0.10, 0.10, 1.90}, 1.0, 0.05},
    };

    otis::Vector3D P_predict = {0.0, 0.0, 2.0};

    std::cout << "--- Scenario 1: Normal Flight (3 frames) ---\n\n";
    for (int i = 1; i <= 3; ++i) {
        double nablaV = 0.02 + 0.01 * i;
        std::vector<double> no_features = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        engine.process_frame(segments, P_predict, nablaV, no_features);
        kalman.update(P_predict, 0.033);
        P_predict = kalman.predict(0.033);
    }

    std::cout << "\n--- Scenario 2: FlyTrap DPA Attack (3 frames) ---\n\n";
    for (int i = 1; i <= 3; ++i) {
        double nablaV = 0.40 + 0.05 * i;
        std::vector<double> adv_features = {0.88, 0.76, 0.93, 0.71, 0.82, 0.67};
        engine.process_frame(segments, P_predict, nablaV, adv_features);
        kalman.update(P_predict, 0.033);
        P_predict = kalman.predict(0.033);
    }

    std::cout << "\n--- Session Summary ---\n";
    std::cout << "  Total frames     : " << engine.get_frame_count()   << "\n";
    std::cout << "  Anomalies flagged: " << engine.get_anomaly_count() << "\n";

    if (engine.get_anomaly_count() > 0) {
        std::cout << "\n[O.T.I.S.] PASS — FlyTrap DPA detected by beta(nablaV).\n";
    } else {
        std::cout << "\n[O.T.I.S.] FAIL — No anomalies detected.\n";
        return 1;
    }

    return 0;
}
