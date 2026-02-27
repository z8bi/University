import numpy as np
import matplotlib.pyplot as plt

# Measured currents (µA)
I_TP1 = np.array([11.54, 25.40, 45.50, 64.96, 85.63, 104.8])  # ammeter @ TP1
I_RB1 = np.array([9.30, 20.68, 37.45, 53.66, 70.85, 87.45])   # from Vdrop/47k

def false_zero_limits(y, pad_frac=0.08):
    ymin, ymax = float(np.min(y)), float(np.max(y))
    span = max(ymax - ymin, 1e-9)
    pad = span * pad_frac
    return ymin - pad, ymax + pad

plt.figure()
plt.scatter(I_RB1, I_TP1)

# Ideal line (if they were equal): y = x
xline = np.linspace(min(I_RB1.min(), I_TP1.min()),
                    max(I_RB1.max(), I_TP1.max()), 200)
plt.plot(xline, xline, linestyle="--", label="Ideal: I_TP1 = I_RB1")

plt.xlabel("Current through RB1 (µA)")
plt.ylabel("Current through TP1 (µA)")
plt.title("Current Mirror: Output Current vs Reference Current")
plt.xlim(*false_zero_limits(I_RB1))
plt.ylim(*false_zero_limits(I_TP1))
plt.grid(True, linestyle="--", linewidth=0.6)
plt.legend()
plt.tight_layout()

plt.savefig("Current_Mirror_TP1_vs_RB1.png", dpi=300)
plt.show()