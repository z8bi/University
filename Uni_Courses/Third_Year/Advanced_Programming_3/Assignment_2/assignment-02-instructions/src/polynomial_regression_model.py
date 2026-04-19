import numpy as np
import matplotlib.pyplot as plt
from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score

# --- Part 1: Synthetic Cubic Data ---
np.random.seed(42)
n_samples = 100
poly_true = lambda x: x * (x - 1) * (x + 1)
x_data = np.random.uniform(-1.5, 1.5, n_samples)
y_true = poly_true(x_data)
noise = np.random.normal(0, 0.2, size=n_samples) * y_true
y_data = y_true + noise

# Generate polynomial features
poly_features = PolynomialFeatures(degree=3, include_bias=False)
X_data = x_data.reshape((-1, 1))
X_poly = poly_features.fit_transform(X_data)

# Train model
model = LinearRegression()
model.fit(X_poly, y_data)

# Visualization (Added from .qmd requirements)
xx = np.linspace(-1.5, 1.5, 1000)
XX = poly_features.transform(xx.reshape(-1, 1))

plt.figure(figsize=(5, 3.5))
plt.plot(xx, poly_true(xx), 'k--', label="hidden manifold - true model")
plt.plot(xx, model.predict(XX), label='polynomial regression model')
plt.scatter(x_data, y_data, marker='x', s=6, c='r')
plt.legend()
plt.grid(True)
plt.show()

# --- Part 2: Concrete Data Revisited (Polynomial) ---
df = pd.read_csv("../data/concrete.csv")
data = df.to_numpy()
X, y = data[:,:-1], data[:,-1]

poly_feature_second_degree = PolynomialFeatures(degree=2, include_bias=False)
X_poly_second_degree = poly_feature_second_degree.fit_transform(X)

X_train, X_test, y_train, y_test = train_test_split(X_poly_second_degree, y, test_size=0.2, random_state=42)

model_poly = LinearRegression()
model_poly.fit(X_train, y_train)

# Calculate results for verification
r2_train = r2_score(y_train, model_poly.predict(X_train))
r2_test = r2_score(y_test, model_poly.predict(X_test))
print(f"Concrete Polynomial (Degree 2) R2 - Train: {r2_train:.4f}, Test: {r2_test:.4f}")