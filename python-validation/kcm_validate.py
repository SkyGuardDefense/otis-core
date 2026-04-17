"""
kcm_validate.py — FlyTrap Pattern Phase-Edge Validation
SkyGuard Defense Systems / InSitu Labs
otis-core v0.1 — April 2026

Runs decorrelation stretch + phase-edge analysis on FlyTrap adversarial
pattern images. Demonstrates structural edge isolation independent of
optical/color content. Output images saved to results/
"""

import numpy as np
import os

try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False
    print("[WARNING] OpenCV not installed. Install with: pip install opencv-python")

os.makedirs("results", exist_ok=True)
os.makedirs("test_images", exist_ok=True)

# --- Decorrelation Stretch (SDM Stage 1) ---
def decorrelation_stretch(image):
    """Per-channel eigenvector decorrelation. Suppresses adversarial spectral patterns."""
    img_float = image.astype(np.float64)
    h, w, c = img_float.shape
    flat = img_float.reshape(-1, c)
    mean = flat.mean(axis=0)
    centered = flat - mean
    cov = np.cov(centered.T)
    eigenvalues, eigenvectors = np.linalg.eigh(cov)
    stretch = eigenvectors @ np.diag(1.0 / np.sqrt(eigenvalues + 1e-8)) @ eigenvectors.T
    result = (centered @ stretch.T) + mean
    result = np.clip(result, 0, 255).reshape(h, w, c).astype(np.uint8)
    return result

# --- Phase-Edge Detection (KCM Stage 2) ---
def phase_edge_response(image):
    """
    Approximates phase congruency via spatial gradient of illumination-normalized signal.
    Implements: structuralResponse = |dFdx(phaseEdge)| + |dFdy(phaseEdge)|
    Invariant to linear illumination changes — matches TopoLight GLSL shader logic.
    """
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY).astype(np.float32)
    # Normalize illumination
    blurred = cv2.GaussianBlur(gray, (21, 21), 0)
    normalized = gray / (blurred + 1e-6)
    # Spatial gradients (dFdx, dFdy equivalent)
    dx = cv2.Sobel(normalized, cv2.CV_32F, 1, 0, ksize=3)
    dy = cv2.Sobel(normalized, cv2.CV_32F, 0, 1, ksize=3)
    structural_response = np.abs(dx) + np.abs(dy)
    # Normalize for visualization
    response_vis = cv2.normalize(structural_response, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U)
    return structural_response, response_vis

# --- Volume Anomaly Score (KCM beta term) ---
def compute_volume_score(edge_response, theta_morph=0.10, kappa=1000.0, beta_0=1.0):
    """
    Computes beta(nablaV) penalty score from edge response magnitude.
    High score = geometric discontinuity consistent with FlyTrap DPA signature.
    """
    nablaV = float(np.mean(edge_response) / (np.max(edge_response) + 1e-8))
    if nablaV > theta_morph:
        score = beta_0 * nablaV * kappa
    else:
        score = 0.0
    return nablaV, score

# --- Main Validation Pipeline ---
def validate_image(image_path):
    print(f"\n[O.T.I.S. KCM Validate] Processing: {image_path}")
    image = cv2.imread(image_path)
    if image is None:
        print(f"  [ERROR] Could not load image: {image_path}")
        return

    basename = os.path.splitext(os.path.basename(image_path))[0]

    # Stage 1 — SDM decorrelation stretch
    decorrected = decorrelation_stretch(image)
    cv2.imwrite(f"results/{basename}_sdm.png", decorrected)
    print(f"  [SDM] Decorrelation stretch complete → results/{basename}_sdm.png")

    # Stage 2 — KCM phase-edge response
    edge_float, edge_vis = phase_edge_response(decorrected)
    edge_color = cv2.applyColorMap(edge_vis, cv2.COLORMAP_INFERNO)
    cv2.imwrite(f"results/{basename}_kcm_edge.png", edge_color)
    print(f"  [KCM] Phase-edge response complete → results/{basename}_kcm_edge.png")

    # Beta(nablaV) anomaly score
    nablaV, score = compute_volume_score(edge_float)
    flag = "⚠ SPOOFING SIGNATURE DETECTED" if score > 0 else "✓ No anomaly"
    print(f"  [KCM] nablaV={nablaV:.4f}  beta_score={score:.2f}  → {flag}")

    # Side-by-side comparison
    h = min(image.shape[0], 400)
    scale = h / image.shape[0]
    w = int(image.shape[1] * scale)
    orig_resized = cv2.resize(image, (w, h))
    edge_resized = cv2.resize(edge_color, (w, h))
    comparison = np.hstack([orig_resized, edge_resized])
    cv2.imwrite(f"results/{basename}_comparison.png", comparison)
    print(f"  [KCM] Comparison saved → results/{basename}_comparison.png")

# --- Generate synthetic FlyTrap test pattern if no images provided ---
def generate_synthetic_flytrap(output_path="test_images/synthetic_flytrap.png"):
    """
    Generates a synthetic adversarial pattern approximating FlyTrap DPA geometry:
    concentric high-contrast rings engineered to confuse optical flow trackers.
    """
    size = 512
    img = np.zeros((size, size, 3), dtype=np.uint8)
    center = (size // 2, size // 2)
    for r in range(10, 240, 20):
        color = (255, 255, 255) if (r // 20) % 2 == 0 else (0, 0, 0)
        cv2.circle(img, center, r, color, 15)
    # Add gradient overlay to simulate spatiotemporal consistency
    gradient = np.linspace(0, 60, size, dtype=np.uint8)
    overlay = np.tile(gradient, (size, 1))
    img[:, :, 2] = np.clip(img[:, :, 2].astype(int) + overlay, 0, 255).astype(np.uint8)
    cv2.imwrite(output_path, img)
    print(f"[O.T.I.S.] Synthetic FlyTrap pattern generated → {output_path}")
    return output_path

# --- Entry Point ---
if __name__ == "__main__":
    if not HAS_CV2:
        print("[O.T.I.S.] OpenCV required. Run: pip install opencv-python")
        exit(1)

    import sys
    image_files = sys.argv[1:] if len(sys.argv) > 1 else []

    if not image_files:
        print("[O.T.I.S.] No images provided. Generating synthetic FlyTrap test pattern...")
        synthetic = generate_synthetic_flytrap()
        image_files = [synthetic]

    for path in image_files:
        validate_image(path)

    print("\n[O.T.I.S. KCM Validate] Complete. Results in ./results/")
