// volume_penalty.glsl — Volumetric Boundary Anomaly Detection Shader
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026
//
// Computes beta(nablaV) volume continuity penalty per fragment.
// High output = geometric discontinuity inconsistent with rigid body motion.
// FlyTrap DPA signature: high volumetric delta with low momentum error.

precision highp float;

uniform sampler2D uDepthTexture;    // Stereo depth map (normalized 0-1)
uniform sampler2D uPrevDepthTexture; // Previous frame depth map
uniform float uThetaMorph;          // default 0.10
uniform float uKappa;               // default 1000.0
uniform float uBeta0;               // default 1.0
uniform vec2 uResolution;

varying vec2 vTexCoord;

float sampleDepth(sampler2D tex, vec2 uv) {
    return texture2D(tex, uv).r;
}

void main() {
    float currDepth = sampleDepth(uDepthTexture, vTexCoord);
    float prevDepth = sampleDepth(uPrevDepthTexture, vTexCoord);

    // nablaV: normalized volumetric boundary delta between frames
    float nablaV = abs(currDepth - prevDepth) / (prevDepth + 0.001);

    // beta(nablaV) penalty
    float betaScore = 0.0;
    if (nablaV > uThetaMorph) {
        betaScore = uBeta0 * nablaV * uKappa;
    }

    // Normalize for visualization (0-1 range, clamped)
    float normalized = clamp(betaScore / (uKappa * uBeta0), 0.0, 1.0);

    // Color map: green (safe) → yellow (borderline) → red (anomaly)
    vec3 safeColor    = vec3(0.0, 1.0, 0.2);
    vec3 borderColor  = vec3(1.0, 1.0, 0.0);
    vec3 anomalyColor = vec3(1.0, 0.1, 0.0);

    vec3 finalColor;
    if (normalized < 0.5) {
        finalColor = mix(safeColor, borderColor, normalized * 2.0);
    } else {
        finalColor = mix(borderColor, anomalyColor, (normalized - 0.5) * 2.0);
    }

    gl_FragColor = vec4(finalColor, 1.0);
}
