APP_RUN: $(APP_SO)
	${TOOL_BIN_PATH}/qemu-aarch64 -L /usr/aarch64-linux-gnu -E LD_LIBRARY_PATH=${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/third_party:${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/host-x86_64-O2:./ ${OUT_ROOT}/${MAPLE_BUILD_TYPE}/ops/mplsh -Xbootclasspath:libcore-all.so -cp $(APP_SO) $(APP)
