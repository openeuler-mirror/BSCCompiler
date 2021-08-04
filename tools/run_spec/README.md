# Build and Run SPEC with Maple

## Build and test SPEC with maplefe as the front end
```shell
git clone https://gitee.com/openarkcompiler/OpenArkCompiler.git oac   # optional
cd oac
git pull                        # optional for updating Maple code
source build/envsetup.sh arm release
make setup
make
cd tools/run_spec/
make setup                      # copy spec2017 to $HOME, and copy spec cfg file
source envsetup.sh mplfe
make test.602

# the following are optional for running the train size or ref size
make train.605                  
make ref.605
```

## Build and test SPEC with clang2mpl as the front end
```shell
git clone https://gitee.com/openarkcompiler/OpenArkCompiler.git oac   # optional
cd oac
git checkout spec-clang2mpl     # switch a temp branch
git pull                        # optional for updating Maple code
source build/envsetup.sh arm release
make setup
make
make clang2mpl
cd tools/run_spec/
source envsetup.sh clang2mpl
make setup                      # copy spec2017 to $HOME, and copy spec cfg file
make test.602

# the following are optional for running the train size or ref size
make train.605                  
make ref.605
```

## Quick Patch to Run QEMU on Ubuntu 20.04
```shell
sudo apt udpate && sudo apt install -y qemu qemu-user qemu-user-binfmt
sudo cp $MAPLE_ROOT/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/lib/ld-linux-aarch64.so.1 /lib
```