1. setup spec2017 which copy spec2017 to $HOME
make setup

2. setup env
source envsetup.sh

3. build and test
for single benchmark 605.mcf_s:

make build.605
make test.605
make train.605
make ref.605

