ifndef MAPLE_ROOT
  $(error MAPLE_ROOT is not set. Please source zeiss/envsetup.sh to set the build environment)
endif

help:
	@echo "================================================================"
	@echo "1. setup spec2017 source"
	@echo "  make setup"
	@echo ""
	@echo "2. setup env"
	@echo "  source envsetup.sh"
	@echo ""
	@echo "3. build and run, eg for benchmark 605.mcf_s:"
	@echo "  make build.605" 
	@echo "  make test.605"
	@echo "  make train.605"
	@echo "  make ref.605"
	@echo "================================================================"

setup:
	cp -pr /maple/spec2017 $(HOME)
	cp -f $(MAPLE_ROOT)/build/tools/spec/maplec.cfg $(HOME)/spec2017/config

build.%:
	runcpu --config=maplec.cfg --action build $* --rebuild

test.%:
	runcpu --config=maplec.cfg -i test --action run $*

train.%:
	runcpu --config=maplec.cfg -i train --action run $*

ref.%:
	runcpu --config=maplec.cfg -i ref --action run $*


