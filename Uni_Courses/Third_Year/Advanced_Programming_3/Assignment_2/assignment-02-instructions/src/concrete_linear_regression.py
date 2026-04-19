import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score

# Load data
df = pd.read_csv("../data/concrete.csv") 
data = df.to_numpy()
print(f"First 5 rows of this concrete dataset:\n {df.head()}")

# Extract features and labels
X = data[:,:-1]
y = data[:,-1]

# Split dataset
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.20, random_state=42)

# Train the model
model = LinearRegression()
model.fit(X_train, y_train)

# Assessment using R2-score
r2_score_train = r2_score(y_true=y_train, y_pred=model.predict(X_train))
r2_score_test = r2_score(y_true=y_test, y_pred=model.predict(X_test))
print(f"Training set: R2-score: {r2_score_train:.4f}")
print(f"    Test set: R2-score: {r2_score_test:.4f}")