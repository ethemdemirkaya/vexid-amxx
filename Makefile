REAPI_ROOT ?= ../../Derleyici/reapi-5.29.0.358/reapi
CXX ?= g++

TARGET := bin/linux/vexid_amxx_i386.so
SOURCE := src/vexid_module.cpp
TEST_DIR := tests/native_harness/build/linux
MOCK_ENGINE := $(TEST_DIR)/engine_i486.so
TEST_HARNESS := $(TEST_DIR)/vexid_harness

INCLUDES := \
	-I$(REAPI_ROOT)/src \
	-I$(REAPI_ROOT)/include \
	-I$(REAPI_ROOT)/include/metamod \
	-I$(REAPI_ROOT)/include/cssdk/common \
	-I$(REAPI_ROOT)/include/cssdk/dlls \
	-I$(REAPI_ROOT)/include/cssdk/engine \
	-I$(REAPI_ROOT)/include/cssdk/game_shared \
	-I$(REAPI_ROOT)/include/cssdk/pm_shared \
	-I$(REAPI_ROOT)/include/cssdk/public

CXXFLAGS := -m32 -std=c++17 -O2 -fPIC -fvisibility=hidden -Wall -Wextra -Werror \
	-Wno-unknown-pragmas -Wno-strict-aliasing \
	-D_GNU_SOURCE -DHAVE_STRONG_TYPEDEF -DNDEBUG $(INCLUDES)
LDFLAGS := -m32 -shared -Wl,--no-undefined -ldl

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(MOCK_ENGINE): tests/native_harness/mock_swds.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@ -m32 -shared -Wl,--no-undefined,-soname,engine_i486.so

$(TEST_HARNESS): tests/native_harness/vexid_harness.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@ -m32 -ldl

test: $(TARGET) $(MOCK_ENGINE) $(TEST_HARNESS)
	cd $(TEST_DIR) && LD_LIBRARY_PATH=. ./vexid_harness ./engine_i486.so ../../../../bin/linux/vexid_amxx_i386.so

clean:
	rm -f $(TARGET) $(MOCK_ENGINE) $(TEST_HARNESS)
