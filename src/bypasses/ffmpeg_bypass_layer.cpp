// ffmpeg_bypass_layer.cpp — Codec Bypass Layer
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026

#include "otis_core/otis_core.hpp"
#include <cstring>
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <chrono>

namespace otis {
namespace bypass {

// ─────────────────────────────────────────────────────────────────────────────
// BypassStatus
// ─────────────────────────────────────────────────────────────────────────────

enum class BypassStatus : int {
    OK                    = 0,
    ERR_NULL_INPUT        = -1,
    ERR_ZERO_LENGTH       = -2,
    ERR_MALFORMED_HEADER  = -3,
    ERR_EXPLOIT_SIGNATURE = -4,
    ERR_OUTPUT_TOO_SMALL  = -5,
    ERR_CODEC_UNSUPPORTED = -6,
    ERR_SANITIZE_FAILED   = -7,
    ERR_RECONSTRUCT_FAIL  = -8,
};

const char* bypass_status_str(BypassStatus s) {
    switch (s) {
        case BypassStatus::OK:                    return "OK";
        case BypassStatus::ERR_NULL_INPUT:        return "ERR_NULL_INPUT";
        case BypassStatus::ERR_ZERO_LENGTH:       return "ERR_ZERO_LENGTH";
        case BypassStatus::ERR_MALFORMED_HEADER:  return "ERR_MALFORMED_HEADER";
        case BypassStatus::ERR_EXPLOIT_SIGNATURE: return "ERR_EXPLOIT_SIGNATURE";
        case BypassStatus::ERR_OUTPUT_TOO_SMALL:  return "ERR_OUTPUT_TOO_SMALL";
        case BypassStatus::ERR_CODEC_UNSUPPORTED: return "ERR_CODEC_UNSUPPORTED";
        case BypassStatus::ERR_SANITIZE_FAILED:   return "ERR_SANITIZE_FAILED";
        case BypassStatus::ERR_RECONSTRUCT_FAIL:  return "ERR_RECONSTRUCT_FAIL";
        default:                                   return "ERR_UNKNOWN";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// FrameMetadata
// ─────────────────────────────────────────────────────────────────────────────

struct FrameMetadata {
    uint32_t width         = 0;
    uint32_t height        = 0;
    uint32_t channels      = 3;      // 1=gray, 3=RGB, 4=RGBA
    uint32_t bit_depth     = 8;      // 8 or 16
    uint64_t timestamp_us  = 0;
    uint32_t frame_index   = 0;
    bool     is_stereo_left= true;
    bool     sanitized     = false;
    uint8_t  codec_id      = 0;      // 0=raw,1=H264,2=H265,3=MJPEG,4=stereo_raw
};

// ─────────────────────────────────────────────────────────────────────────────
// Known Exploit Signatures
// ─────────────────────────────────────────────────────────────────────────────

static const std::vector<std::vector<uint8_t>> EXPLOIT_SIGNATURES = {
    {0x00, 0x00, 0x00, 0x01, 0x67, 0xFF, 0xFF, 0xFF},  // H264 malformed NAL
    {0x6D, 0x6F, 0x6F, 0x76, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF},  // CVE-2023-38697 class
    {0x80, 0x60, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00},  // GStreamer RTP malformed
    {0x00, 0x00, 0x01, 0x80},                            // NAL forbidden_zero_bit set
};

// ─────────────────────────────────────────────────────────────────────────────
// scan_exploit_signatures
// ─────────────────────────────────────────────────────────────────────────────

static BypassStatus scan_exploit_signatures(const uint8_t* data, size_t length) {
    for (const auto& sig : EXPLOIT_SIGNATURES) {
        if (sig.size() > length) continue;
        for (size_t i = 0; i <= length - sig.size(); ++i) {
            if (std::memcmp(data + i, sig.data(), sig.size()) == 0) {
                std::cerr << "[BYPASS ⚠] Exploit signature at offset " << i
                          << " — stream rejected.\n";
                return BypassStatus::ERR_EXPLOIT_SIGNATURE;
            }
        }
    }
    return BypassStatus::OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// validate_frame_header
// ─────────────────────────────────────────────────────────────────────────────

static BypassStatus validate_frame_header(const FrameMetadata& meta,
                                          size_t data_length) {
    if (meta.width == 0 || meta.height == 0 || meta.channels == 0) {
        std::cerr << "[BYPASS] Invalid dimensions: "
                  << meta.width << "x" << meta.height
                  << "x" << meta.channels << "\n";
        return BypassStatus::ERR_MALFORMED_HEADER;
    }
    if (meta.width > 16384 || meta.height > 16384) {
        std::cerr << "[BYPASS ⚠] Suspicious dimensions — overflow attack suspected.\n";
        return BypassStatus::ERR_MALFORMED_HEADER;
    }
    if (meta.codec_id == 0) {
        size_t expected = static_cast<size_t>(meta.width)
                        * meta.height
                        * meta.channels
                        * (meta.bit_depth / 8);
        if (data_length < expected) {
            std::cerr << "[BYPASS] Data shorter than declared geometry: "
                      << data_length << " < " << expected << "\n";
            return BypassStatus::ERR_MALFORMED_HEADER;
        }
    }
    if (meta.bit_depth != 8 && meta.bit_depth != 16) {
        std::cerr << "[BYPASS] Unsupported bit depth: " << meta.bit_depth << "\n";
        return BypassStatus::ERR_MALFORMED_HEADER;
    }
    if (meta.codec_id > 4) {
        std::cerr << "[BYPASS] Unsupported codec_id: "
                  << static_cast<int>(meta.codec_id) << "\n";
        return BypassStatus::ERR_CODEC_UNSUPPORTED;
    }
    return BypassStatus::OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// sanitize_frame_buffer
// ─────────────────────────────────────────────────────────────────────────────

static BypassStatus sanitize_frame_buffer(
    const uint8_t*  data,
    size_t          length,
    const FrameMetadata& meta,
    uint8_t*        output,
    size_t          output_capacity,
    size_t&         bytes_written)
{
    if (!data || !output)  return BypassStatus::ERR_NULL_INPUT;
    if (length == 0)        return BypassStatus::ERR_ZERO_LENGTH;

    size_t frame_bytes = static_cast<size_t>(meta.width)
                       * meta.height
                       * meta.channels
                       * (meta.bit_depth / 8);

    if (output_capacity < frame_bytes) {
        std::cerr << "[BYPASS] Output buffer too small: "
                  << output_capacity << " < " << frame_bytes << "\n";
        return BypassStatus::ERR_OUTPUT_TOO_SMALL;
    }

    size_t copy_bytes = std::min(length, frame_bytes);
    std::memcpy(output, data, copy_bytes);

    if (copy_bytes < frame_bytes)
        std::memset(output + copy_bytes, 0, frame_bytes - copy_bytes);

    if (meta.bit_depth == 16) {
        uint16_t* depth = reinterpret_cast<uint16_t*>(output);
        size_t pixel_count = static_cast<size_t>(meta.width)
                           * meta.height * meta.channels;
        for (size_t i = 0; i < pixel_count; ++i) {
            if (depth[i] > 60000) depth[i] = 0;
        }
    }

    bytes_written = frame_bytes;
    return BypassStatus::OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// process_stream — Full bypass pipeline entry point
// ─────────────────────────────────────────────────────────────────────────────

BypassStatus process_stream(
    const uint8_t* raw_input,
    size_t         input_length,
    FrameMetadata& meta,
    uint8_t*       clean_output,
    size_t         output_capacity,
    size_t&        bytes_written)
{
    bytes_written = 0;
    if (!raw_input)       return BypassStatus::ERR_NULL_INPUT;
    if (input_length == 0) return BypassStatus::ERR_ZERO_LENGTH;

    auto t_start = std::chrono::high_resolution_clock::now();

    BypassStatus s = scan_exploit_signatures(raw_input, input_length);
    if (s != BypassStatus::OK) {
        std::cerr << "[BYPASS] REJECTED at signature scan: "
                  << bypass_status_str(s) << "\n";
        return s;
    }

    s = validate_frame_header(meta, input_length);
    if (s != BypassStatus::OK) {
        std::cerr << "[BYPASS] REJECTED at header validation: "
                  << bypass_status_str(s) << "\n";
        return s;
    }

    s = sanitize_frame_buffer(raw_input, input_length,
                               meta, clean_output,
                               output_capacity, bytes_written);
    if (s != BypassStatus::OK) {
        std::cerr << "[BYPASS] REJECTED at sanitization: "
                  << bypass_status_str(s) << "\n";
        return s;
    }

    meta.sanitized    = true;
    meta.timestamp_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());

    auto t_end = std::chrono::high_resolution_clock::now();
    double latency_us = std::chrono::duration<double, std::micro>(
                            t_end - t_start).count();

    std::cout << "[BYPASS ✓] Frame " << meta.frame_index
              << " | " << meta.width << "x" << meta.height
              << " | codec=" << static_cast<int>(meta.codec_id)
              << " | " << bytes_written << " bytes"
              << " | latency=" << latency_us << "µs"
              << " | CLEAN\n";

    return BypassStatus::OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// run_bypass_selftest
// ─────────────────────────────────────────────────────────────────────────────

void run_bypass_selftest() {
    std::cout << "\n[BYPASS] === Self-Test Suite ===\n\n";
    int passed = 0, failed = 0;

    auto check = [&](const char* name, BypassStatus got, BypassStatus expected) {
        bool ok = (got == expected);
        std::cout << (ok ? "  ✓ " : "  ✗ ") << name
                  << " — got=" << bypass_status_str(got)
                  << " expected=" << bypass_status_str(expected) << "\n";
        ok ? passed++ : failed++;
    };

    // Test 1 — null input
    {
        FrameMetadata meta{};
        uint8_t out[16]; size_t bw = 0;
        check("null input",
              process_stream(nullptr, 100, meta, out, sizeof(out), bw),
              BypassStatus::ERR_NULL_INPUT);
    }

    // Test 2 — zero length
    {
        FrameMetadata meta{};
        uint8_t in[4] = {0,0,0,0};
        uint8_t out[16]; size_t bw = 0;
        check("zero length",
              process_stream(in, 0, meta, out, sizeof(out), bw),
              BypassStatus::ERR_ZERO_LENGTH);
    }

    // Test 3 — exploit signature
    {
        std::vector<uint8_t> bad = {
            0x00, 0x11,
            0x00, 0x00, 0x00, 0x01, 0x67, 0xFF, 0xFF, 0xFF,
            0x33
        };
        FrameMetadata meta{640, 480, 3, 8, 0, 0, true, false, 1};
        std::vector<uint8_t> out(640*480*3); size_t bw = 0;
        check("exploit signature rejected",
              process_stream(bad.data(), bad.size(),
                             meta, out.data(), out.size(), bw),
              BypassStatus::ERR_EXPLOIT_SIGNATURE);
    }

    // Test 4 — zero dimensions
    {
        uint8_t in[64] = {0};
        FrameMetadata meta{0, 480, 3, 8, 0, 1, true, false, 0};
        uint8_t out[64]; size_t bw = 0;
        check("zero dimension rejected",
              process_stream(in, sizeof(in), meta, out, sizeof(out), bw),
              BypassStatus::ERR_MALFORMED_HEADER);
    }

    // Test 5 — clean frame happy path
    {
        const uint32_t W=4, H=4, C=3;
        std::vector<uint8_t> in(W*H*C, 128);
        FrameMetadata meta{W, H, C, 8, 0, 2, true, false, 0};
        std::vector<uint8_t> out(W*H*C); size_t bw = 0;
        check("clean frame passes",
              process_stream(in.data(), in.size(),
                             meta, out.data(), out.size(), bw),
              BypassStatus::OK);
    }

    std::cout << "\n[BYPASS] " << passed << " passed / " << failed << " failed\n";
    if (failed == 0)
        std::cout << "[BYPASS] ✓ All self-tests PASSED\n";
    else
        std::cout << "[BYPASS] ✗ FAILURES — review bypass layer\n";
}

} // namespace bypass
} // namespace otis
