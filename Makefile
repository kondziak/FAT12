.PHONY: clean All

All:
	@echo "----------Building project:[ MyFAT - Debug ]----------"
	@cd "MyFAT" && "$(MAKE)" -f  "MyFAT.mk"
clean:
	@echo "----------Cleaning project:[ MyFAT - Debug ]----------"
	@cd "MyFAT" && "$(MAKE)" -f  "MyFAT.mk" clean
