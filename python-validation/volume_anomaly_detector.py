"""
volume_anomaly_detector.py — Frame-by-Frame Beta(nablaV) Anomaly Scoring
SkyGuard Defense Systems / InSitu Labs
otis-core v0.1 — April 2026

Processes a video file frame-by-frame. Computes volumetric boundary
delta (nablaV) between consecutive frames and flags frames that exceed
theta_morph threshold — the FlyTrap DPA geometric signature.
"""

import numpy as np
import os
import sys

try:
    import cv2
    HAS_CV2 = True
except ImportError:
    HAS_CV2 = False

os.makedirs("results", exist_ok=True)

THETA_MORPH = 0.10
KAPPA = 1000.0
BETA_0 = 1.0

def compute_edge_volume(frame):
    """Estimate volumetric boundary density from frame edge response."""
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY).astype(np.float32)
    blurred = cv2.GaussianBlur(gray, (21, 21), 0)
    normalized = gray / (blurred + 1e-6)
    dx = cv2.Sobel(normalized, cv2.CV_32F, 1, 0, ksize=3)
    dy = cv2.Sobel(normalized, cv2.CV_32F, 0, 1, ksize=3)
    edge_magnitude = np.abs(dx) + np.abs(dy)
    return float(np.mean(edge_magnitude))

def beta_penalty(nablaV):
    if nablaV > THETA_MORPH:
        return BETA_0 * nablaV * KAPPA
    return 0.0

def analyze_video(video_path):
    print(f"\n[O.T.I.S. Volume Detector] Input: {video_path}")
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"[ERROR] Cannot open video: {video_path}")
        return

    fps = cap.get(cv2.CAP_PROP_FPS)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    print(f"[VST] {total_frames} frames @ {fps:.1f} fps")

    prev_volume = None
    frame_idx = 0
    anomaly_frames = []
    scores = []

    while True:
        ret, frame = cap.read()
        if not ret:
            break
        frame_idx += 1
        curr_volume = compute_edge_volume(frame)

        if prev_volume is not None:
            nablaV = abs(curr_volume - prev_volume) / (prev_volume + 1e-8)
            score = beta_penalty(nablaV)
            scores.append(score)
            timestamp = frame_idx / fps

            if score > 0:
                anomaly_frames.append((frame_idx, timestamp, nablaV, score))
                flag = f"⚠ ANOMALY  nablaV={nablaV:.4f}  beta={score:.1f}"
            else:
                flag = f"  OK       nablaV={nablaV:.4f}"

            if frame_idx % 30 == 0 or score > 0:
                print(f"  Frame {frame_idx:04d} | t={timestamp:.2f}s | {flag}")

        prev_volume = curr_volume

    cap.release()

    print(f"\n[O.T.I.S.] Analysis complete.")
    print(f"  Total frames analyzed : {frame_idx}")
    print(f"  Anomaly frames flagged: {len(anomaly_frames)}")
    if scores:
        print(f"  Peak beta score       : {max(scores):.2f}")
        print(f"  Mean beta score       : {np.mean(scores):.4f}")

    # Save anomaly report
    report_path = "results/volume_anomaly_report.txt"
    with open(report_path, "w") as f:
        f.write("O.T.I.S. Volume Anomaly Detector — Report\n")
        f.write(f"Input: {video_path}\n")
        f.write(f"theta_morph={THETA_MORPH}  kappa={KAPPA}\n\n")
        f.write(f"{'Frame':>8} {'Time(s)':>10} {'nablaV':>10} {'beta_score':>12} {'Flag':>30}\n")
        for fr, ts, nv, sc in anomaly_frames:
            f.write(f"{fr:>8} {ts:>10.2f} {nv:>10.4f} {sc:>12.2f} {'SPOOFING SIGNATURE':>30}\n")
        if not anomaly_frames:
            f.write("No anomalies detected.\n")
    print(f"  Report saved → {report_path}")

if __name__ == "__main__":
    if not HAS_CV2:
        print("[O.T.I.S.] OpenCV required. Run: pip install opencv-python")
        exit(1)

    if len(sys.argv) < 2:
        print("Usage: python volume_anomaly_detector.py <video_path>")
        print("Example: python volume_anomaly_detector.py insitu_poc.mp4")
        exit(1)

    analyze_video(sys.argv[1])
