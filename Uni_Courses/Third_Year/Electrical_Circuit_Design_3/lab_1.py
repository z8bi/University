import numpy as np
import matplotlib.pyplot as plt

# -----------------------------
# All measured data (unsorted)
# -----------------------------
freq_khz = np.array([
    100, 200, 400, 600, 800, 1000,
    820, 840, 860, 870, 875, 878, 885, 890
], dtype=float)

gain_db = np.array([
    -6.2, -11.2, -13.5, -14.5, -9.11, -13.9,
    -6.9, -2.2, 0.5, 3.7, 5.1, 5.26, 4.05, 2.36
], dtype=float)

phase_deg = np.array([
    -99.4, -87.6, -71.4, -72.3, -78.7, 97.9,
    -75.6, -65.7, -52.2, -31.1, -9.8, 0.3, 31.0, 48.6
], dtype=float)

# -----------------------------
# Sort data by increasing frequency
# -----------------------------
sort_idx = np.argsort(freq_khz)

freq_khz  = freq_khz[sort_idx]
gain_db   = gain_db[sort_idx]
phase_deg = phase_deg[sort_idx]

# -----------------------------
# Resonant point (max gain)
# -----------------------------
i_res = np.nanargmax(gain_db)
f_res = freq_khz[i_res]
g_res = gain_db[i_res]
p_res = phase_deg[i_res]

# -----------------------------
# Bode plot
# -----------------------------
fig, (ax_gain, ax_phase) = plt.subplots(
    2, 1, sharex=True, figsize=(9, 7), constrained_layout=True
)

# Font sizes
LABEL_FS = 14
TICK_FS = 12
ANNOT_FS = 12
TITLE_FS = 20

# Gain plot
ax_gain.semilogx(freq_khz, gain_db, marker='o', linewidth=1.8)
ax_gain.set_ylabel("Gain (dB)", fontsize=LABEL_FS)
ax_gain.grid(True, which="both")
ax_gain.tick_params(labelsize=TICK_FS)

# Mark resonance
ax_gain.axvline(f_res, linestyle='--', linewidth=1.5)
ax_gain.plot(f_res, g_res, marker='o', markersize=10)
ax_gain.annotate(
    f"Resonance\n{f_res:.0f} kHz, {g_res:.2f} dB",
    xy=(f_res, g_res),
    xytext=(12, -28),
    textcoords="offset points",
    fontsize=ANNOT_FS,
    arrowprops=dict(arrowstyle="->", lw=1.2),
)

# Phase plot
ax_phase.semilogx(freq_khz, phase_deg, marker='o', linewidth=1.8)
ax_phase.set_xlabel("Frequency (kHz)", fontsize=LABEL_FS)
ax_phase.set_ylabel("Phase (degrees)", fontsize=LABEL_FS)
ax_phase.grid(True, which="both")
ax_phase.tick_params(labelsize=TICK_FS)

# Mark resonance
ax_phase.axvline(f_res, linestyle='--', linewidth=1.5)
ax_phase.plot(f_res, p_res, marker='o', markersize=10)
ax_phase.annotate(
    f"Resonance\n{f_res:.0f} kHz, {p_res:.1f}°",
    xy=(f_res, p_res),
    xytext=(12, -28),
    textcoords="offset points",
    fontsize=ANNOT_FS,
    arrowprops=dict(arrowstyle="->", lw=1.2),
)

# -----------------------------
# Custom x-axis ticks (hundreds)
# -----------------------------
xticks = [100, 200, 400, 600, 800, 1000]
ax_phase.set_xticks(xticks)
ax_phase.set_xticklabels([str(x) for x in xticks], fontsize=TICK_FS)

# -----------------------------
# Title
# -----------------------------
plt.suptitle(
    "Bode Plot for Colpitts Oscillator",
    fontsize=TITLE_FS,
    fontweight="bold",
    y=0.965
)

# -----------------------------
# Save PNG (high quality)
# -----------------------------
plt.savefig(
    "bode_plot_colpitts_oscillator.png",
    dpi=300,
    bbox_inches="tight"
)

plt.show()
