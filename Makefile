BUILD_DIR_RELEASE=.build_release
BUILD_DIR_DEBUG=.build_debug
COMPILE_COMMANDS_DIR=.compile_commands

NPROC=$(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

all:
	@echo "Usage:"
	@echo "    make <release | .debug>"
	@echo "    make <check>"
	@echo "    make <clean>"

release:
	@mkdir -p $(BUILD_DIR_RELEASE)
	cd $(BUILD_DIR_RELEASE) && cmake -DCMAKE_BUILD_TYPE=Release .. && $(MAKE) -j$(NPROC)
	cp $(BUILD_DIR_RELEASE)/texpacker .

.debug:
	@mkdir -p $(BUILD_DIR_DEBUG)
	cd $(BUILD_DIR_DEBUG) && cmake -DCMAKE_BUILD_TYPE=Debug .. && $(MAKE) -j$(NPROC)
	cp $(BUILD_DIR_DEBUG)/texpacker .

check:
	cppcheck -j $(NPROC) --std=c++17 --check-level=exhaustive --enable=all -f -I src src/ 2> cppcheck-output

build_compile_commands:
	@mkdir -p $(COMPILE_COMMANDS_DIR)
	cd $(COMPILE_COMMANDS_DIR) && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1
	rm -f compile_commands.json && ln -s $(COMPILE_COMMANDS_DIR)/compile_commands.json compile_commands.json

clean:
	rm -fr $(BUILD_DIR_RELEASE) $(BUILD_DIR_DEBUG) texpacker
	rm -fr $(COMPILE_COMMANDS_DIR) .cache/
