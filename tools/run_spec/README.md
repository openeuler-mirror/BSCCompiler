# Build and Run SPEC with Maple

1. Setup and build OAC

```
# Clone the repo
git clone https://gitee.com/openarkcompiler/OpenArkCompiler.git oac
# Or alternatively update the repo
cd oac
git pull

# Setup the environment
source build/envsetup.sh arm release
make setup

# Build OAC
make
# If using clang2mpl, build it
make clang2mpl
```

2. Setup the environment for running SPEC

```
cd tools/run_spec
make setup
source envsetup.sh [ clang2mpl | hir2mpl ] # select frontend
```

3. Run a test

```
make test.602

# Build only
make build.600

# Run the train or ref sizes
make train.605
make ref.605
```

## Quick Patch to Run QEMU on Ubuntu 20.04
```shell
sudo apt udpate && sudo apt install -y qemu qemu-user qemu-user-binfmt
sudo cp $MAPLE_ROOT/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/ld-linux-aarch64.so.1 /lib
```
