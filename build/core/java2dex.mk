$(APP_DEX): %.dex : %.java $(wildcard $(JAR_LIBS_PATH)/*.java)
	$(JAVA2DEX) -o $(APP_DEX) $(JAVA2DEX_FLAGS) -i $(APP_JAVA):${EXTRA_JAVA_FILE} $<
