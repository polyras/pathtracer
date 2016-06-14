include MakefileSettings

all: debug

COMMON_FLAGS = -Wall -std=c++11 -ferror-limit=1 -fno-exceptions -fno-rtti
COMPILE_FLAGS = -iquote $(CODE_ROOT)
release: COMMON_FLAGS += -O2
debug: COMMON_FLAGS += -O0 -DDEBUG -g

PRODUCT_DIR = $(BUILD_DIR)/products
OBJ_DIR = $(BUILD_DIR)/objects
ROOT = $(realpath ./..)
CODE_ROOT = $(ROOT)/code

OBJ_CPP_SOURCES = osx_main.mm
CPP_SOURCES = rendering.cpp game.cpp lib/assert.cpp
CPP_OBJS = $(patsubst %.cpp, %.o, $(CPP_SOURCES))
OBJ_CPP_OBJS = $(patsubst %.mm, %.o, $(OBJ_CPP_SOURCES))
OBJS = $(OBJ_CPP_OBJS) $(CPP_OBJS)

DEBUG_OBJ_DIR = $(OBJ_DIR)/debug
DEBUG_OBJS = $(addprefix $(DEBUG_OBJ_DIR)/, $(OBJS))
DEBUG_DEPS = $(sort $(patsubst %, %.deps, $(DEBUG_OBJS)))

RELEASE_OBJ_DIR = $(OBJ_DIR)/release
RELEASE_OBJS = $(addprefix $(RELEASE_OBJ_DIR)/, $(OBJS))
RELEASE_DEPS = $(sort $(patsubst %, %.deps, $(RELEASE_OBJS)))

OSX_FRAMEWORKS = CoreFoundation AppKit OpenGL
OSX_FRAMEWORKS_FLAGS = $(addprefix -framework , $(OSX_FRAMEWORKS))

-include $(DEBUG_DEPS)
-include $(RELEASE_DEPS)

DEBUG_BINARY = $(PRODUCT_DIR)/DebugPathtracer.app/MacOS/DebugPathtracer
RELEASE_BINARY = $(PRODUCT_DIR)/Pathtracer.app/MacOS/Pathtracer

define CREATE_CPP_OBJ_COMMAND
mkdir -p $(dir $@)
$(CXX) $(COMMON_FLAGS) $(COMPILE_FLAGS) -c $< -o $@ -MMD -MF $@.deps
endef

define CREATE_OBJ_CPP_OBJ_COMMAND
mkdir -p $(dir $@)
$(CXX) $(COMMON_FLAGS) $(COMPILE_FLAGS) -fno-objc-arc -c $< -o $@ -MMD -MF $@.deps
endef

define CREATE_BINARY_COMMAND
mkdir -p $(dir $@)
$(CXX) $(COMMON_FLAGS) $(OSX_FRAMEWORKS_FLAGS) $^ -o $@
endef

$(DEBUG_OBJ_DIR)/%.o: $(CODE_ROOT)/%.cpp
	$(CREATE_CPP_OBJ_COMMAND)

$(DEBUG_OBJ_DIR)/%.o: $(CODE_ROOT)/%.mm
	$(CREATE_OBJ_CPP_OBJ_COMMAND)

$(RELEASE_OBJ_DIR)/%.o: $(CODE_ROOT)/%.cpp
	$(CREATE_CPP_OBJ_COMMAND)

$(RELEASE_OBJ_DIR)/%.o: $(CODE_ROOT)/%.mm
	$(CREATE_OBJ_CPP_OBJ_COMMAND)

$(DEBUG_BINARY): $(DEBUG_OBJS)
	$(CREATE_BINARY_COMMAND)

$(RELEASE_BINARY): $(RELEASE_OBJS)
	$(CREATE_BINARY_COMMAND)

debug: $(DEBUG_BINARY)
release: $(RELEASE_BINARY)

clean:
	rm -rf $(BUILD_DIR)

rd: debug
	$(DEBUG_BINARY)

rr: release
	$(RELEASE_BINARY)
