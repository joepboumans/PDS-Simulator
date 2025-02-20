import struct
import os
import pandas as pd
from pandas.core.series import fmt
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from utils import *
from statsmodels.nonparametric.smoothers_lowess import lowess

def plotWMRE(data):
    # Set the figure size
    plt.figure(figsize=(10, 6))
    
    # Create a consistent palette mapping for the unique DataStructName groups.
    unique_names = data['DataSetName'].unique()
    palette = sns.color_palette("deep", n_colors=len(unique_names))
    color_map = dict(zip(unique_names, palette))

    # Create a line style mapping for TupleType.
    unique_tuple = sorted(data['TupleType'].unique())
    styles = ['-', '--', '-.', ':']
    line_style_map = {tt: styles[i % len(styles)] for i, tt in enumerate(unique_tuple)}

    # dashes = {tt: (5,2) if line_style_map[tt] == '-' else (2,2) for tt in unique_tuple}
    # Create a line plot for Epoch vs Weighted Mean Relative Error
    sns.lineplot(
        x='Total Time',                          # X-axis: Epoch
        y='Weighted Mean Relative Error',      # Y-axis: Weighted Mean Relative Error
        hue='DataSetName',               # Group by DataStructType
        style='DataStructName',
        data=data,               # Data to plot
        palette=color_map,
    )

    plt.xlabel('Estimation Time (ms)')
    plt.ylabel('Weighted Mean Relative Error')
    plt.title('Weighted Mean Relative Error over time')
    plt.legend(title='Data Structures')
    plt.tight_layout()
    plt.savefig('plots/epoch_vs_weighted_error.png', dpi=300, bbox_inches='tight')

    plt.figure(figsize=(10, 6))
    # # Create a consistent palette mapping for the unique DataStructName groups.
    unique_names = data['DataStructName'].unique()
    palette = sns.color_palette("deep", n_colors=len(unique_names))
    color_map = dict(zip(unique_names, palette))

    # Create a scatterplot plot for Epoch vs Weighted Mean Relative Error
    sns.scatterplot(
        x='Total Time',                          # X-axis: Epoch
        y='Weighted Mean Relative Error',      # Y-axis: Weighted Mean Relative Error
        hue='DataStructName',               # Group by DataStructType
        style='TupleType',
        data=data,               # Data to plot
        palette=color_map,
        s=10
    )

    # Loop over each combination of DataStructName and TupleType for LOWESS smoothing.
    for (ds, tt), group in data.groupby(['DataStructName', 'TupleType']):
        sorted_group = group.sort_values(by='Total Time')
        if len(sorted_group) < 2:
            continue  # Skip groups with insufficient data for smoothing.
        smoothed = lowess(
            sorted_group['Weighted Mean Relative Error'], 
            sorted_group['Total Time'], 
            frac=0.1
        )
        plt.plot(smoothed[:, 0], smoothed[:, 1], 
                 label=f"{ds}, {tt} (smoothed)", 
                 linewidth=2, 
                 color=color_map[ds],
                 linestyle=line_style_map[tt])

    # Determine the maximum Total Time for the shortest dataset
    shortest_max = data.groupby('DataStructName')['Total Time'].max().min()
    # Set the x-axis limits: from the minimum Total Time to the shortest dataset's max
    plt.xlim(data['Total Time'].min(), shortest_max)
    # Add labels and title
    plt.xlabel('Estimation Time (ms)')
    plt.ylabel('Weighted Mean Relative Error')
    plt.title('Weighted Mean Error over time')
    plt.legend(title='Data Structures')
    plt.tight_layout()
    plt.savefig('plots/epoch_vs_weighted_error.png', dpi=300, bbox_inches='tight')

def plotAvgWMRE(data):
    # Set the figure size
    plt.figure(figsize=(10, 6))
    # Group 'em' data by TupleType, DataStructType, and DataStructName
    grouped_data = data.groupby(['TupleType', 'DataStructType', 'DataStructName'])['Weighted Mean Relative Error'].mean().reset_index()
    print("Grouped 'em' Data:")
    print(grouped_data)

    # Create a line plot for Epoch vs Weighted Mean Relative Error
    sns.boxplot(
        x='DataStructName',                          # X-axis: Epoch
        y='Weighted Mean Relative Error',      # Y-axis: Weighted Mean Relative Error
        hue='DataStructName',               # Group by DataStructType
        # style='TupleType',                  # Distinguish by TupleType
        data=grouped_data               # Data to plot
    )

    # Add labels and title
    plt.xlabel('Epoch')
    plt.ylabel('Weighted Mean Relative Error')
    plt.title('Average Weighted Mean Relative Error (Estimation Data)')
    plt.legend(title='Data Structure Type')
    plt.tight_layout()
    plt.savefig('plots/avg_weighted_error.png', dpi=300, bbox_inches='tight')

def plotEntropy(data):
    # Set the figure size
    plt.figure(figsize=(10, 6))

    # Create a line plot for Epoch vs Weighted Mean Relative Error
    sns.lineplot(
        x='Epoch',                          # X-axis: Epoch
        y='Entropy',      # Y-axis: Weighted Mean Relative Error
        hue='DataStructName',               # Group by DataStructType
        style='TupleType',                  # Distinguish by TupleType
        data=data               # Data to plot
    )

    # Add labels and title
    plt.xlabel('Epoch')
    plt.ylabel('Entropy')
    plt.title('Entropy')
    plt.legend(title='Data Structure Type')
    plt.tight_layout()
    plt.savefig('plots/entropy.png', dpi=300, bbox_inches='tight')

def plotCard(data):
    # Set the figure size
    plt.figure(figsize=(10, 6))

    # Create a line plot for Epoch vs Weighted Mean Relative Error
    sns.lineplot(
        x='Epoch',                          # X-axis: Epoch
        y='Cardinality',      # Y-axis: Weighted Mean Relative Error
        hue='DataStructName',               # Group by DataStructType
        style='TupleType',                  # Distinguish by TupleType
        data=data               # Data to plot
    )

    # Add labels and title
    plt.xlabel('Epoch')
    plt.ylabel('Cardinality')
    plt.title('Cardinality')
    plt.legend(title='Data Structure Type')
    plt.tight_layout()
    plt.savefig('plots/cardinality.png', dpi=300, bbox_inches='tight')

def plotEstimationTime(data):
    filtered_data = data[data['Epoch'] != 0]
    print("Filtered data:")
    print(filtered_data.head())

# Set the figure size
    plt.figure(figsize=(10, 6))
# Create a line plot for Epoch vs Estimation Time
    sns.lineplot(
        x='Epoch',                          # X-axis: Epoch
        y='Estimation Time',                 # Y-axis: Estimation Time
        hue='DataStructName',               # Group by DataStructType
        style='TupleType',                  # Distinguish by TupleType
        data=filtered_data               # Data to plot
    )

# Add labels and title
    plt.xlabel('Epoch')
    plt.ylabel('Estimation Time (ms)')
    plt.title('Epoch vs Estimation Time (Estimation Data)')
    plt.legend(title='PDS, Tuple type')
    plt.tight_layout()
    plt.savefig('plots/epoch_vs_estimation_time.png', dpi=300, bbox_inches='tight')

def plotTotalEstimationTime(data):
# Set up the figure and subplots
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))  # 1 row, 2 columns
    filtered_data = data[data['Epoch'] != 0]

# Left Subplot: Total Time
    sns.boxplot(
        x='DataStructName',                 # X-axis: Data Structure Type
        y='Total Time',                      # Y-axis: Total Time
        hue='TupleType',                    # Group by TupleType
        data=filtered_data,              # Data to plot
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
        data=filtered_data,              # Data to plot
        ax=axes[1]                          # Assign to the right subplot
    )
    axes[1].set_xlabel('Data Structure Type')
    axes[1].set_ylabel('Estimation Time (ms)')
    axes[1].set_title('Distribution of Estimation Time')
    axes[1].legend(title='PDS, Tuple Type')

# Adjust layout and show the plot
    plt.tight_layout()

# Save the plots
    plt.savefig('plots/time_distribution_boxplot.png', dpi=300, bbox_inches='tight')

def plotNS(data):
    # Set the figure size
    plt.figure(figsize=(10, 6))

    # Create a line plot for Epoch vs Weighted Mean Relative Error
    sns.lineplot(
        x="Flow Size",
        y="Frequency",
        # hue="Epoch",
        # style="DataStructName",
        hue="DataStructName",
        markers=True,
        data=data
    )

    # Add labels and title
    plt.legend(title='Data Structure Type')
    plt.xscale("log")
    plt.xlabel("Value")
    plt.yscale("log")
    plt.ylabel("Frequency")
    plt.title("Frequency Plot")
    plt.tight_layout()
    plt.savefig('plots/fsd.png', dpi=300, bbox_inches='tight')

def plotNSdelta(data):
    # Set the figure size
    plt.figure(figsize=(10, 6))

    # Create a line plot for Epoch vs Weighted Mean Relative Error
    sns.lineplot(
        x="Flow Size",
        y="Delta",
        # hue="Epoch",
        # style="DataStructName",
        hue="DataStructName",
        markers=True,
        data=data
    )

    # Add labels and title
    plt.legend(title='Data Structure Type')
    plt.xscale("log")
    plt.xlabel("Flow Size")
    # plt.yscale("log")
    plt.ylabel("Delta")
    plt.title("Frequency Plot")
    plt.tight_layout()
    plt.savefig('plots/delta_fsd.png', dpi=300, bbox_inches='tight')

def read_binary_file(filename):
    flow_sizes = []
    
    with open(filename, 'rb') as f:
        while True:
            # Read the length of the flow size distribution (integer)
            length_bytes = f.read(4)
            if not length_bytes:
                break  # End of file

            length = struct.unpack('I', length_bytes)[0]
            print(f"Found row with length of {length}")
            
            fmt_sz = struct.calcsize("=Id")
            # Read the flow size distribution (doubles)
            flow_data = f.read(length * fmt_sz)  # Each double is 8 bytes
            if len(flow_data) != length * fmt_sz:
                print(f"Not enough data in {filename}. Data length was {len(flow_data)}, needed {length * fmt_sz}")
                continue

            values_counts = list(struct.iter_unpack(f'=Id', flow_data))
            flow_sizes.append(values_counts)
    
    # print(flow_sizes)
    return flow_sizes

def main():
    # set_test_rcs()
# Root directory
    root_dir = "results"

# Lists to hold all dataframes
    dataframes = []  # For non-'em' CSV files
    em_dataframes = []  # For 'em' CSV files
    ns_dataframes = []

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

                                    # Extract dataset name from the filename
                                    dataset_name = file_name.split("_")[1] if file_name.startswith('em_') else file_name.split("_")[0]
                                    data['DataSetName'] = dataset_name.replace(".dat", "")
                                    
                                    # Separate 'em' files from the rest
                                    if file_name.startswith('em'):
                                        em_dataframes.append(data)
                                    else:
                                        dataframes.append(data)
                                elif file_name.endswith(".dat"):
                                    file_path = os.path.join(data_struct_name_dir, file_name)
                                    data = read_binary_file(file_path)

                                    data_pd = pd.DataFrame(
                                                        data[-1],
                                                        columns=["Flow Size", "Frequency"]
                                    )

                                    # data_pd = pd.DataFrame(
                                    #                     [(value, freq, i) for i, dataset in enumerate(data) for value, freq in dataset],
                                    #                     columns=["Flow Size", "Frequency", "Epoch"]
                                    # )
                                    # Add metadata columns for TupleType, DataStructType, and DataStructName
                                    data_pd['TupleType'] = tuple_type
                                    data_pd['DataStructType'] = data_struct_type
                                    data_pd['DataStructName'] = data_struct_name
                                    ns_dataframes.append(data_pd)

                        else:
                            file_path = data_struct_name_dir
                            if file_path.endswith('.dat'):
                                data = read_binary_file(file_path)

                                data_pd = pd.DataFrame(
                                                    data[0],
                                                    columns=["Flow Size", "Frequency"]
                                )

                                # data_pd = pd.DataFrame(
                                #                     [(value, freq, i) for i, dataset in enumerate(data) for value, freq in dataset],
                                #                     columns=["Flow Size", "Frequency", "Epoch"]
                                # )
                                # Add metadata columns for TupleType, DataStructType, and DataStructName
                                data_pd['TupleType'] = tuple_type
                                data_pd['DataStructType'] = data_struct_type
                                data_pd['DataStructName'] = "TrueData"
                                ns_dataframes.append(data_pd)



    # Display the combined data
    print("Non-Estimation Data:")
    combined_data = pd.concat(dataframes, ignore_index=True)
    print(combined_data.head())

    print("\nEstimation Data:")
    combined_em_data = pd.concat(em_dataframes, ignore_index=True)
    print(combined_em_data.head())

    # print("\nFSD Data:")
    # combined_ns_data = pd.concat(ns_dataframes, ignore_index=True)
    # true_df = combined_ns_data[combined_ns_data["DataStructName"] == "TrueData"][["Flow Size", "Frequency"]].rename(
    #     columns={"Frequency": "TrueFrequency"}
    # )
    # print(true_df.head())
    # combined_ns_data = combined_ns_data.merge(true_df, on="Flow Size", how="left")
    # combined_ns_data["Delta"] = combined_ns_data["Frequency"] - combined_ns_data["TrueFrequency"]
    # combined_ns_data.loc[combined_ns_data["DataStructName"] == "TrueData", "Delta"] = 0
    # print(combined_ns_data.head())
    # Group non-'em' data by TupleType, DataStructType, and DataStructName
    grouped_data = combined_data.groupby(['TupleType', 'DataStructType', 'DataStructName'])['F1 Member'].mean().reset_index()
    print("Grouped Non-'em' Data:")
    print(grouped_data)

    print(combined_em_data['DataStructName'].unique())
    filtered_data = combined_em_data[combined_em_data['TupleType'] == "FlowTuple"]
    test_data = filtered_data[filtered_data['DataStructName'] == "FCM-Sketches"]
    print(test_data.head())

    for dsName in filtered_data['DataSetName'].unique():
        print(test_data[test_data['DataSetName'] == dsName])
        print(dsName)
        filteredDsData = filtered_data[filtered_data['DataSetName'] == dsName]
        print(filteredDsData['DataStructName'].unique())
        exit(1)
        print(filteredDsData['DataStructName'].unique())
        plotWMRE(filteredDsData)
        plt.show()
    # plotAvgWMRE(combined_em_data)
    # plotEntropy(combined_em_data)
    # plotCard(combined_em_data)
    # plotEstimationTime(combined_em_data)
    # plotTotalEstimationTime(combined_em_data)
    # plotNS(combined_ns_data)
    # plotNSdelta(combined_ns_data)



if __name__ == "__main__":
    main()
