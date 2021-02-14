# Installation

## Install bazel (Google Build System)

First, install java.
Ubuntu 16.04 (LTS) uses OpenJDK 8 by default:
```
sudo apt install openjdk-8-jdk
```

Ubuntu 18.04 (LTS) uses OpenJDK 11 by default:
```
sudo apt install openjdk-11-jdk
```

Install required packages:
```
sudo apt install g++ unzip zip
```

Then, install Bazel (tested with 3.1.0):

```bash
wget https://github.com/bazelbuild/bazel/releases/download/3.1.0/bazel-3.1.0-installer-linux-x86_64.sh
chmod +x bazel-3.1.0-installer-linux-x86_64.sh
./bazel-3.1.0-installer-linux-x86_64.sh --user
```

The Bazel executable is installed in your `$HOME/bin` directory. It’s a good idea to add this directory to your default paths, as follows:
```
export PATH="$PATH:$HOME/bin"
```
You can also add this command to your ~/.bashrc or ~/.zshrc file to make it permanent.

For for information: [http://www.bazel.io/docs/install.html]

## Install prerequisites

### Ubuntu 16.04
```
sudo apt-get install libgoogle-glog-dev libgflags-dev libmicrohttpd-dev libcurl4-openssl-dev libcurl3 libssl-dev libargtable2-dev cmake
```

### Ubuntu 18.04 and newer
```
sudo apt-get install libgoogle-glog-dev libgflags-dev libmicrohttpd-dev libcurl4-openssl-dev libssl-dev
```

### Z3
You will also need to install the Z3-solver: [https://github.com/Z3Prover/z3].
Current implementation is tested with Z3 4.6.2. To use it checkout the following version before installation:

```
git clone https://github.com/Z3Prover/z3.git
cd z3
git checkout 998cf4c726258fa10900cdb905eda246a8d15148
python scripts/mk_make.py
cd build
make
sudo make install
```


# Compile

```
./build.sh
```
