// otis_core.hpp — O.T.I.S. Top-Level Pipeline Header
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026
//
// Includes all O.T.I.S. subsystem headers and defines the top-level
// pipeline configuration. Include this header for full system access.

#pragma once

#include "kcm_math.hpp"
#include <cstdint>
#include <vector>

namespace otis {

// ─────────────────────────────────────────────────────────────────────────────
// Pipeline Configuration — matches MATH.md defaults
// ─────────────────────────────────────────────────────────────────────────────

struct OTISConfig {
    // KCM weights
    double alpha       = 1.0;     // Momentum continuity weight
    double beta0       = 1.0;     // Volume penalty base
    double gamma       = 1.0;     // Feature discard weight
    double theta_morph = 0.10;    // Volumetric shift threshold (10%)
    double kappa       = 1000.0;  // Penalty multiplier

    // Kalman predictor
    double kalman_process_noise     = 0.01;
    double kalman_measurement_noise = 0.1;

    // Bypass layer
    bool   enable_exploit_scan     = true;
    bool   enable_header_validation= true;
    bool   enable_sanitization     = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// KCM_Engine — Alias for top-level access (matches ARCHITECTURE.md)
// ─────────────────────────────────────────────────────────────────────────────

using KCM_Engine = KCMEngine;

} // namespace otis

// ─────────────────────────────────────────────────────────────────────────────
// Bypass Layer — Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

namespace otis {
namespace bypass {

enum class BypassStatus : int;
struct FrameMetadata;

BypassStatus process_stream(
    const uint8_t* raw_input,
    size_t input_length,
    FrameMetadata& meta,
    uint8_t* clean_output,
    size_t output_capacity,
    size_t& bytes_written);

void run_bypass_selftest();

} // namespace bypass
} // namespace otis
