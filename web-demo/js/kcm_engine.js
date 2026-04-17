// kcm_engine.js — KCM WebGL Engine
// SkyGuard Defense Systems / InSitu Labs
// otis-core v0.1 — April 2026
//
// Connects GLSL shader output to KCM scoring pipeline.
// Manages WebGL context, shader loading, texture updates, and beta(nablaV) scoring.

'use strict';

const KCMEngine = (() => {

    // KCM Parameters (match MATH.md)
    const THETA_MORPH = 0.10;
    const KAPPA = 1000.0;
    const BETA_0 = 1.0;
    const ALPHA = 1.0;
    const EDGE_THRESHOLD = 0.15;
    const EDGE_INTENSITY = 3.0;

    let gl = null;
    let canvas = null;
    let shaderProgram = null;
    let volumeProgram = null;
    let currentTexture = null;
    let prevDepthTexture = null;
    let frameCount = 0;
    let anomalyLog = [];

    // --- WebGL Init ---
    function init(canvasId) {
        canvas = document.getElementById(canvasId);
        gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
        if (!gl) {
            console.error('[KCM] WebGL not supported.');
            return false;
        }
        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
        console.log('[KCM Engine] Initialized. WebGL ready.');
        return true;
    }

    // --- Shader Loader ---
    async function loadShader(url, type) {
        const response = await fetch(url);
        const source = await response.text();
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error(`[KCM] Shader compile error (${url}):`, gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }
        return shader;
    }

    async function buildProgram(vertUrl, fragUrl) {
        const vert = await loadShader(vertUrl, gl.VERTEX_SHADER);
        const frag = await loadShader(fragUrl, gl.FRAGMENT_SHADER);
        if (!vert || !frag) return null;
        const program = gl.createProgram();
        gl.attachShader(program, vert);
        gl.attachShader(program, frag);
        gl.linkProgram(program);
        if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
            console.error('[KCM] Program link error:', gl.getProgramInfoLog(program));
            return null;
        }
        return program;
    }

    // --- Texture Management ---
    function createTextureFromImage(image) {
        const tex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, tex);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, image);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        return tex;
    }

    function updateTextureFromVideo(videoElement) {
        if (!currentTexture) currentTexture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, currentTexture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, videoElement);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    }

    // --- Beta(nablaV) JS scoring (CPU-side validation) ---
    function computeBetaScore(edgePixels) {
        // edgePixels: Float32Array of normalized edge response values
        const mean = edgePixels.reduce((a, b) => a + b, 0) / edgePixels.length;
        const max = Math.max(...edgePixels);
        const nablaV = max > 0 ? mean / max : 0;
        const score = nablaV > THETA_MORPH ? BETA_0 * nablaV * KAPPA : 0;
        return { nablaV, score };
    }

    // --- Render Frame ---
    function renderFrame(lightPosition = [0.0, 2.0, 3.0]) {
        if (!gl || !shaderProgram) return;
        gl.viewport(0, 0, canvas.width, canvas.height);
        gl.clearColor(0.0, 0.0, 0.0, 1.0);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
        gl.useProgram(shaderProgram);

        // Set uniforms
        const uLight = gl.getUniformLocation(shaderProgram, 'uLightPosition');
        const uThresh = gl.getUniformLocation(shaderProgram, 'uEdgeThreshold');
        const uIntensity = gl.getUniformLocation(shaderProgram, 'uEdgeIntensity');
        const uRes = gl.getUniformLocation(shaderProgram, 'uResolution');
        gl.uniform3fv(uLight, lightPosition);
        gl.uniform1f(uThresh, EDGE_THRESHOLD);
        gl.uniform1f(uIntensity, EDGE_INTENSITY);
        gl.uniform2f(uRes, canvas.width, canvas.height);

        frameCount++;
    }

    // --- Anomaly Event System ---
    function logAnomaly(frameIdx, nablaV, betaScore) {
        const event = {
            frame: frameIdx,
            timestamp: Date.now(),
            nablaV: nablaV.toFixed(4),
            betaScore: betaScore.toFixed(2),
            flag: 'FLYTRAP_DPA_SIGNATURE'
        };
        anomalyLog.push(event);
        console.warn(`[KCM ⚠] Frame ${frameIdx} | nablaV=${event.nablaV} | beta=${event.betaScore} | SPOOFING DETECTED`);
        document.dispatchEvent(new CustomEvent('otis-anomaly', { detail: event }));
    }

    function getAnomalyLog() { return [...anomalyLog]; }
    function getFrameCount() { return frameCount; }

    // --- Public API ---
    return {
        init,
        buildProgram,
        createTextureFromImage,
        updateTextureFromVideo,
        computeBetaScore,
        renderFrame,
        logAnomaly,
        getAnomalyLog,
        getFrameCount,
        THETA_MORPH,
        KAPPA,
        ALPHA
    };

})();

window.KCMEngine = KCMEngine;
