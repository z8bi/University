import numpy as np
import matplotlib.pyplot as plt

# ---- Data ----
Vp = np.array([5.09, 10.12, 15.09, 20.15, 25.00, 30.08, 35.10, 40.14, 45.09])
Ip = np.array([0.0169, 0.0294, 0.0424, 0.0574, 0.0754, 0.1022, 0.1473, 0.3018, 1.5420])
THD = np.array([2.24, 4.21, 6.64, 9.39, 12.84, 18.38, 28.14, 54.29, 93.47])

def false_zero_limits(y, pad_frac=0.08):
    ymin, ymax = float(np.min(y)), float(np.max(y))
    span = max(ymax - ymin, 1e-9)
    pad = span * pad_frac
    return ymin - pad, ymax + pad

# ---- Knee point (first break in linearity) ----
knee_idx = np.where(Vp == 35.10)[0][0]
knee_voltage = Vp[knee_idx]
knee_current = Ip[knee_idx]
knee_thd = THD[knee_idx]

# ---- Plot 1: Voltage vs Current ----
plt.figure()
plt.plot(Ip, Vp, marker='o')
plt.scatter(knee_current, knee_voltage)
plt.axhline(knee_voltage, linestyle='--')

plt.text(knee_current, knee_voltage - 2.0,
         f"KNEE ≈ {knee_voltage:.2f} V",
         color='red', fontweight='bold')

plt.xlabel("Primary Current (A)")
plt.ylabel("Primary Input Voltage (V)")
plt.title("Primary Input Voltage vs Primary Current")
plt.ylim(*false_zero_limits(Vp))
plt.grid(True, linestyle='--', linewidth=0.6)
plt.tight_layout()

plt.savefig("Primary_Voltage_vs_Primary_Current.png", dpi=300)


# ---- Plot 2: Voltage vs THD ----
plt.figure()
plt.plot(THD, Vp, marker='o')
plt.scatter(knee_thd, knee_voltage)
plt.axhline(knee_voltage, linestyle='--')

plt.text(knee_thd, knee_voltage - 2.0,
         f"KNEE ≈ {knee_voltage:.2f} V",
         color='red', fontweight='bold')

plt.xlabel("Primary Current THD (%)")
plt.ylabel("Primary Input Voltage (V)")
plt.title("Primary Input Voltage vs Primary Current THD")
plt.ylim(*false_zero_limits(Vp))
plt.grid(True, linestyle='--', linewidth=0.6)
plt.tight_layout()

plt.savefig("Primary_Voltage_vs_Primary_THD.png", dpi=300)
