# %%
import pandas as pd
import matplotlib.pyplot as plt

# %% Load the dataset


# %%
df = pd.read_csv('./data/mnist_micro.csv', header=None)

# Shuffle the data so that we can visualize various digits
df_shuffled = df.sample(frac=1, random_state=42).reset_index(drop=True)

# Split features and labels -- in numpy arrays
X = df_shuffled.iloc[:,:-1].values
y = df_shuffled.iloc[:, -1].values

print(f"type(df) = {type(df)}")
print(f"type(X) = {type(X)}")
print(f"type(y) = {type(y)}")

print(f"X.shape = {X.shape}")
print(f"y.shape = {y.shape}")

# %%
fig, axes = plt.subplots(3, 4, figsize=(8, 6))
fig.suptitle("Shuffled Sample Images from Micro MNIST dataset", fontsize=14)
for i, ax in enumerate(axes.flatten()):
    image = X[i].reshape(28, 28)

    # Display the image with magma colormap. 
    # A few colormap to try out: 
    #   viridis (default), plasma, inferno, magma, cividis, gray
    ax.imshow(image, 'magma')
    # avoid floating point presentation by casting to int: int(y[i])
    ax.set_title(f"Label: {int(y[i])}", fontsize=16)
    ax.axis('off')

plt.tight_layout()
plt.show()

# %%
fig, axes = plt.subplots(3, 4, figsize=(8, 6))
fig.suptitle("Shuffled Sample Images from Micro MNIST dataset", fontsize=14)
for i, ax in enumerate(axes.flatten()):
    image = X[i].reshape(28, 28)

    # Display the image with magma colormap. 
    # A few colormap to try out: 
    #   viridis (default), plasma, inferno, magma, cividis, gray
    ax.imshow(image, 'magma')
    # avoid floating point presentation by casting to int: int(y[i])
    ax.set_title(f"Label: {int(y[i])}", fontsize=16)
    ax.axis('off')

plt.tight_layout()
plt.show()