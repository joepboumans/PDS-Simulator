# Network Datastructures Simulator
This simulator takes in datasets of 5-tuples which are fed into your designed or known network datastructures. These structures are can be multitude of Probablisitc Data Structures, like frequency counting (CountMin, FCM Sketch) or Membership Approximation (Bloomfilter, CuckooHash).
## Installation
The dataset downloader is depended on `tshark` as to parse pcap files and transform them into 5-tuple datasets. To install `tshark` use the following:
```
$ sudo apt install tshark
```
The Simulator has no direct dependenies and can be ran by removing the tests from the `CMakeLists.txt`. The tests are depended on `Catch2` and can be installed according to their [cmake integration](https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#installing-catch2-from-git-repository). In short, installing it via git can be done like:
```
$ git clone https://github.com/catchorg/Catch2.git
$ cd Catch2
$ cmake -B build -S . -DBUILD_TESTING=OFF
$ sudo cmake --build build/ --target install
```

## Dataset Downloader
The downloader is used to download pcap dataset from [CAIDA](https://www.caida.org/) and parser these into 5-tuples datasets to save space and computing effort in the simulator. To use the Downloader add your datasets to the `data-sets.txt` filein the format of `passive-YYYY/equinix-XXXX/YYYYMMDD-HHMMSS.UTC`. With your datasets added, now run the downloader by:
```
$ python dataset_downloader.py --user <username>:<password> -i data-sets.txt
```
The dataset downloader will now start downloading the pcap file and parser it into the `.dat` file. This will take a while as the datasets from CAIDA are quite big.

## Simulator
The Simulator can load in multiple data structures and pass the dataset through them like a network switch could. This can be used to evaluate and test structures on their performance before starting to implement P4 code. Beware that P4 brings alot of constrictions and your designed data structures might not work on P4.
The Simulator is build using `cmake` and for the first build use:
```
$ cmake -H. -B build/
```
Whereafter each build can be ran via:
```
$ cmake --build build/
```
