// kcm_fragment.glsl — Phase-Edge Fragment Shader (TopoLight derivative)
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026
//
// Computes illumination-invariant structural edge response via phase congruency
// approximation. Originally developed for archaeological specimen boundary
// detection (InSitu / TopoLight). Applied here to defeat adversarial optical
// spoofing by detecting geometric discontinuities independent of surface color.
//
// Math reference: MATH.md Section 4

precision highp float;

uniform sampler2D uTexture;
uniform vec3 uLightPosition;
uniform float uEdgeThreshold;   // default 0.15
uniform float uEdgeIntensity;   // default 3.0
uniform vec2 uResolution;

varying vec2 vTexCoord;
varying vec3 vPosition;
varying vec3 vNormal;

// Phase-congruency edge response (GLSL approximation)
float phaseEdgeResponse(vec3 normal, vec3 lightPos, vec3 fragPos) {
    vec3 lightDir = normalize(lightPos - fragPos);
    float phaseEdge = dot(normalize(normal), lightDir);
    // dFdx/dFdy: spatial rate-of-change of phase response
    float structuralResponse = abs(dFdx(phaseEdge)) + abs(dFdy(phaseEdge));
    return structuralResponse;
}

// Decorrelation stretch approximation (per-channel normalization)
vec3 spectralDelaminate(vec3 color) {
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    vec3 deviation = color - vec3(luminance);
    float maxDev = max(abs(deviation.r), max(abs(deviation.g), abs(deviation.b)));
    return 0.5 + deviation / (maxDev * 2.0 + 0.001);
}

void main() {
    vec4 texColor = texture2D(uTexture, vTexCoord);

    // Stage 1 — SDM: Spectral delamination
    vec3 sdmColor = spectralDelaminate(texColor.rgb);

    // Stage 2 — KCM: Phase-edge structural response
    float edgeResponse = phaseEdgeResponse(vNormal, uLightPosition, vPosition);

    // Apply threshold — only structural boundaries pass
    float edgeMask = step(uEdgeThreshold, edgeResponse);
    float edgeStrength = edgeMask * edgeResponse * uEdgeIntensity;

    // Overlay edge detection on decorrelation-stretched base
    vec3 edgeColor = vec3(0.0, 1.0, 0.4); // KCM lock color — green
    vec3 finalColor = mix(sdmColor, edgeColor, clamp(edgeStrength, 0.0, 1.0));

    gl_FragColor = vec4(finalColor, texColor.a);
}
