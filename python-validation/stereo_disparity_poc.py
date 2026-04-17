"""
stereo_disparity_poc.py — VST Stage 3 Proof of Concept
SkyGuard Defense Systems / InSitu Labs
otis-core v0.1 — April 2026

OpenCV StereoSGBM depth map generation. Demonstrates Volumetric
Stereo-Triangulation (VST) stage using synthetic or real stereo pairs.
Output: results/disparity_map.png
"""

import numpy as np
import os

try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False

os.makedirs("results", exist_ok=True)
os.makedirs("test_images", exist_ok=True)

def generate_synthetic_stereo_pair(output_dir="test_images"):
    """Generates a synthetic stereo pair by offsetting a test image."""
    size = (480, 640)
    base = np.zeros((size[0], size[1], 3), dtype=np.uint8)
    # Draw geometric shapes simulating a drone silhouette
    cv2.rectangle(base, (200, 150), (440, 330), (180, 180, 180), -1)
    cv2.circle(base, (320, 240), 60, (220, 220, 220), -1)
    cv2.line(base, (100, 240), (540, 240), (140, 140, 140), 8)
    cv2.line(base, (320, 100), (320, 380), (140, 140, 140), 8)
    # Add noise
    noise = np.random.randint(0, 25, base.shape, dtype=np.uint8)
    base = cv2.add(base, noise)
    left = base.copy()
    # Simulate disparity by horizontal pixel offset
    disparity_offset = 24
    M = np.float32([[1, 0, disparity_offset], [0, 1, 0]])
    right = cv2.warpAffine(base, M, (size[1], size[0]))
    left_path = os.path.join(output_dir, "stereo_left.jpg")
    right_path = os.path.join(output_dir, "stereo_right.jpg")
    cv2.imwrite(left_path, left)
    cv2.imwrite(right_path, right)
    print(f"[VST] Synthetic stereo pair generated → {left_path}, {right_path}")
    return left_path, right_path

def compute_disparity(left_path, right_path):
    left = cv2.imread(left_path, cv2.IMREAD_GRAYSCALE)
    right = cv2.imread(right_path, cv2.IMREAD_GRAYSCALE)
    if left is None or right is None:
        print("[VST ERROR] Could not load stereo images.")
        return None

    stereo = cv2.StereoSGBM_create(
        minDisparity=0,
        numDisparities=96,
        blockSize=11,
        P1=8 * 3 * 11 ** 2,
        P2=32 * 3 * 11 ** 2,
        disp12MaxDiff=1,
        uniquenessRatio=15,
        speckleWindowSize=100,
        speckleRange=2,
        mode=cv2.STEREO_SGBM_MODE_SGBM_3WAY
    )

    disparity = stereo.compute(left, right).astype(np.float32) / 16.0
    disp_vis = cv2.normalize(disparity, None, 0, 255, cv2.NORM_MINMAX, cv2.CV_8U)
    disp_color = cv2.applyColorMap(disp_vis, cv2.COLORMAP_MAGMA)
    output_path = "results/disparity_map.png"
    cv2.imwrite(output_path, disp_color)

    print(f"[VST] Disparity range: {disparity.min():.2f} — {disparity.max():.2f}")
    print(f"[VST] Depth map saved → {output_path}")
    return disparity

if __name__ == "__main__":
    if not HAS_CV2:
        print("[O.T.I.S.] OpenCV required. Run: pip install opencv-python")
        exit(1)

    import sys
    if len(sys.argv) == 3:
        left_path, right_path = sys.argv[1], sys.argv[2]
    else:
        print("[VST] No stereo pair provided. Generating synthetic pair...")
        left_path, right_path = generate_synthetic_stereo_pair()

    print(f"\n[O.T.I.S. VST] Processing stereo pair...")
    disparity = compute_disparity(left_path, right_path)
    if disparity is not None:
        print("[O.T.I.S. VST] Stereo disparity PoC complete.")
