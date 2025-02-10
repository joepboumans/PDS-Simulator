import os
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from utils import *

# Root directory
root_dir = "results"

# Lists to hold all dataframes
dataframes = []  # For non-'em' CSV files
em_dataframes = []  # For 'em' CSV files

# Loop through all directories and files
for tuple_type in os.listdir(root_dir):
    tuple_dir = os.path.join(root_dir, tuple_type)
    if os.path.isdir(tuple_dir):
        for data_struct_type in os.listdir(tuple_dir):
            data_struct_dir = os.path.join(tuple_dir, data_struct_type)
            if os.path.isdir(data_struct_dir):
                for data_struct_name in os.listdir(data_struct_dir):
                    data_struct_name_dir = os.path.join(data_struct_dir, data_struct_name)
                    if os.path.isdir(data_struct_name_dir):
                        for file_name in os.listdir(data_struct_name_dir):
                            if file_name.endswith(".csv"):
                                file_path = os.path.join(data_struct_name_dir, file_name)
                                
                                # Load the CSV file
                                data = pd.read_csv(file_path)
                                
                                # Add metadata columns for TupleType, DataStructType, and DataStructName
                                data['TupleType'] = tuple_type
                                data['DataStructType'] = data_struct_type
                                data['DataStructName'] = data_struct_name
                                
                                # Separate 'em' files from the rest
                                if file_name.startswith('em'):
                                    em_dataframes.append(data)
                                else:
                                    dataframes.append(data)

# Combine all dataframes into two separate DataFrames
combined_data = pd.concat(dataframes, ignore_index=True)
combined_em_data = pd.concat(em_dataframes, ignore_index=True)

# Display the combined data
print("Non-Estimation Data:")
print(combined_data.head())

print("\nEstimation Data:")
print(combined_em_data.head())

# Group non-'em' data by TupleType, DataStructType, and DataStructName
grouped_data = combined_data.groupby(['TupleType', 'DataStructType', 'DataStructName'])['F1 Member'].mean().reset_index()
print("Grouped Non-'em' Data:")
print(grouped_data)

# Group 'em' data by TupleType, DataStructType, and DataStructName
grouped_em_data = combined_em_data.groupby(['TupleType', 'DataStructType', 'DataStructName'])['Weighted Mean Relative Error'].mean().reset_index()
print("Grouped 'em' Data:")
print(grouped_em_data)

# Set the figure size
plt.figure(figsize=(10, 6))

# Create a line plot for Epoch vs Weighted Mean Relative Error
sns.lineplot(
    x='Epoch',                          # X-axis: Epoch
    y='Weighted Mean Relative Error',      # Y-axis: Weighted Mean Relative Error
    hue='DataStructName',               # Group by DataStructType
    style='TupleType',                  # Distinguish by TupleType
    data=combined_em_data               # Data to plot
)

# Add labels and title
plt.xlabel('Epoch')
plt.ylabel('Weighted Mean Relative Error')
plt.title('Epoch vs Weighted Mean Relative Error (Estimation Data)')
plt.legend(title='Data Structure Type')


filtered_em_data = combined_em_data[combined_em_data['Epoch'] != 0]
print("Filtered data:")
print(filtered_em_data.head())

# Set the figure size
plt.figure(figsize=(10, 6))
# Create a line plot for Epoch vs Estimation Time
sns.lineplot(
    x='Epoch',                          # X-axis: Epoch
    y='Estimation Time',                 # Y-axis: Estimation Time
    hue='DataStructName',               # Group by DataStructType
    style='TupleType',                  # Distinguish by TupleType
    data=filtered_em_data               # Data to plot
)

# Add labels and title
plt.xlabel('Epoch')
plt.ylabel('Estimation Time (ms)')
plt.title('Epoch vs Estimation Time (Estimation Data)')
plt.legend(title='PDS, Tuple type')

# Set up the figure and subplots
fig, axes = plt.subplots(1, 2, figsize=(14, 6))  # 1 row, 2 columns

# Left Subplot: Total Time
sns.boxplot(
    x='DataStructName',                 # X-axis: Data Structure Type
    y='Total Time',                      # Y-axis: Total Time
    hue='TupleType',                    # Group by TupleType
    data=combined_em_data,              # Data to plot
    ax=axes[0]                          # Assign to the left subplot
)
axes[0].set_xlabel('Data Structure Type')
axes[0].set_ylabel('Total Time (ms)')
axes[0].set_title('Distribution of Total Time')
axes[0].legend(title='PDS, Tuple Type')

# Right Subplot: Estimation Time
sns.boxplot(
    x='DataStructName',                 # X-axis: Data Structure Type
    y='Estimation Time',                 # Y-axis: Estimation Time
    hue='TupleType',                    # Group by TupleType
    data=combined_em_data,              # Data to plot
    ax=axes[1]                          # Assign to the right subplot
)
axes[1].set_xlabel('Data Structure Type')
axes[1].set_ylabel('Estimation Time (ms)')
axes[1].set_title('Distribution of Estimation Time')
axes[1].legend(title='PDS, Tuple Type')

# Adjust layout and show the plot
plt.tight_layout()
plt.show()

# Save the plots
plt.savefig('plots/epoch_vs_weighted_error.png', dpi=300, bbox_inches='tight')
plt.savefig('plots/epoch_vs_estimation_time.png', dpi=300, bbox_inches='tight')
plt.savefig('plots/time_distribution_boxplot.png', dpi=300, bbox_inches='tight')
