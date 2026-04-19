import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import accuracy_score

# Load ECG data
df = pd.read_csv("../data/ecg.csv")
data = df.to_numpy()
X = data[:,:-1]
y = data[:,-1]

# Train and Test Split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.20, random_state=42)
lg_model = LogisticRegression()
lg_model.fit(X_train, y_train)

# Accuracy check
y_pred = lg_model.predict(X_test)
accuracy = accuracy_score(y_test, y_pred)
print(f"Model accuracy: {accuracy: .4f}")