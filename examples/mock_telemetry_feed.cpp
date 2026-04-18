// mock_telemetry_feed.cpp — Multi-Frame Inertial CG Tracking Demo
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026

#include "otis_core/kcm_math.hpp"
#include "otis_core/otis_core.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

int main() {
    std::cout << "=================================================\n";
    std::cout << " O.T.I.S. — Mock Telemetry Feed (CG Tracker)\n";
    std::cout << " SkyGuard Defense Systems — otis-core v0.1\n";
    std::cout << "=================================================\n\n";

    otis::KCMEngine engine(1.0, 1.0, 1.0, 0.10, 1000.0);
    otis::KalmanMomentumPredictor kalman(0.01, 0.1);

    const double dt           = 0.033;
    const int    TOTAL_FRAMES = 10;
    const int    SPOOF_FRAME  = 6;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::setw(8)  << "Frame"
              << std::setw(10) << "nablaV"
              << std::setw(12) << "beta"
              << std::setw(10) << "J"
              << std::setw(22) << "Status" << "\n";
    std::cout << std::string(62, '-') << "\n";

    for (int frame = 1; frame <= TOTAL_FRAMES; ++frame) {
        double t = frame * dt;

        std::vector<otis::TargetMass> segments = {
            {{ 0.10 * std::cos(t), 0.05 * std::sin(t), 2.0 - t * 0.3 }, 1.0, 0.05},
            {{ 0.00,               0.10 * std::sin(t), 2.0 - t * 0.3 }, 1.0, 0.05},
            {{ 0.05 * std::cos(t), 0.00,               2.0 - t * 0.3 }, 1.0, 0.05},
            {{ 0.10 * std::cos(t), 0.10 * std::sin(t), 1.9 - t * 0.3 }, 1.0, 0.05},
        };

        otis::Vector3D P_predict = kalman.predict(dt);

        bool   is_spoof = (frame == SPOOF_FRAME);
        double nablaV   = is_spoof
            ? 0.47
            : (0.02 + 0.005 * std::sin(t * 3.0));

        std::vector<double> features = is_spoof
            ? std::vector<double>{0.91, 0.83, 0.77, 0.88, 0.72, 0.95}
            : std::vector<double>{0.00, 0.00, 0.00, 0.00, 0.00, 0.00};

        otis::KCMResult r = engine.process_frame(
            segments, P_predict, nablaV, features);
        kalman.update(P_predict, dt);

        std::string status = r.spoofing_detected
            ? "FLYTRAP DETECTED"
            : "LOCK NOMINAL";

        std::cout << std::setw(8)  << frame
                  << std::setw(10) << nablaV
                  << std::setw(12) << r.beta_term
                  << std::setw(10) << r.J
                  << std::setw(22) << status << "\n";
    }

    std::cout << "\n--- Session Summary ---\n";
    std::cout << "  Total frames     : " << engine.get_frame_count()   << "\n";
    std::cout << "  Anomalies flagged: " << engine.get_anomaly_count() << "\n";
    std::cout << "  Spoof injected at: frame " << SPOOF_FRAME           << "\n";

    bool pass = (engine.get_anomaly_count() == 1);
    std::cout << "\n[O.T.I.S.] " << (pass ? "PASS" : "FAIL")
              << " — Expected 1 anomaly at frame " << SPOOF_FRAME << "\n";

    return pass ? 0 : 1;
}
