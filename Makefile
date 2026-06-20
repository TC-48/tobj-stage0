# tools
CC ?= cc
AR ?= ar
BUILD ?= release

# utils
rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

ifeq ($(OS),Windows_NT)
	PLATFORM := windows
else
	PLATFORM := posix
endif

ifeq ($(PLATFORM),posix)
	EXE_EXT    :=
	STATIC_EXT := .a
	SHARED_EXT := .so
else ifeq ($(PLATFORM),windows)
	EXE_EXT    := .exe
	STATIC_EXT := .lib
	SHARED_EXT := .dll
endif

# diretories
SRC_DIR     := src
INCLUDE_DIR := include
DEPS_DIR    := deps

TC48_EMU_DIR := $(DEPS_DIR)/tc48-emu

OBJ_ROOT_DIR := build/$(BUILD)/obj
DEP_ROOT_DIR := build/$(BUILD)/dep

OUT_DIR     := out/$(BUILD)
LIB_DIR     := $(OUT_DIR)/lib
BIN_DIR     := $(OUT_DIR)/bin

TOBJ_LIB_NAME   := tobj
TOBJ_LIB_STATIC := $(LIB_DIR)/lib$(TOBJ_LIB_NAME)$(STATIC_EXT)
TOBJ_LIB_SHARED := $(LIB_DIR)/lib$(TOBJ_LIB_NAME)$(SHARED_EXT)
TC48_EMU_LIB    := $(TC48_EMU_DIR)/out/$(BUILD)/lib/libtc48emu.a

# flags
CSTD       := -std=c11
WARNINGS   := -Wall -Wextra -Werror=implicit-fallthrough -Wno-old-style-declaration
PIC_CFLAGS := -fPIC

COMMON_CFLAGS := $(CSTD) $(WARNINGS) -I$(INCLUDE_DIR) -I$(DEPS_DIR)/tc48-emu/include -I$(DEPS_DIR)/tc48-emu/deps

ifeq ($(PLATFORM),windows)
	CMD_MKDIR_P = powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\,$(1))'"
	CMD_RM_RF   = powershell -NoProfile -Command "Remove-Item -Recurse -Force -Path '$(subst /,\,$(1))'"
	CMD_RM_F    = powershell -NoProfile -Command "Remove-Item -Force -Path '$(subst /,\,$(1))'"
	CMD_CP_R    = powershell -NoProfile -Command "Copy-Item -Recurse -Force -Path '$(subst /,\,$(1))' -Destination '$(subst /,\,$(2))'"
	CMD_INSTALL = powershell -NoProfile -Command "Copy-Item -Force -Path '$(subst /,\,$(2))' -Destination '$(subst /,\,$(3))'"
else
	CMD_MKDIR_P = mkdir -p "$(1)"
	CMD_RM_RF   = rm -rf "$(1)"
	CMD_RM_F    = rm -f "$(1)"
	CMD_CP_R    = cp -R "$(1)" "$(2)"
	CMD_INSTALL = install -m $(1) "$(2)" "$(3)"
endif

# sources
TOBJ_SRCS := $(call rwildcard,src/tobj,*.c)

TOBJ_OBJS_STATIC := $(patsubst %.c,$(OBJ_ROOT_DIR)/%.o,$(TOBJ_SRCS))
TOBJ_OBJS_SHARED := $(patsubst %.c,$(OBJ_ROOT_DIR)/shared/%.o,$(TOBJ_SRCS))

TOBJ_LINK_DEP := $(TOBJ_LIB_STATIC)

TOOL_DIRS  := $(sort $(dir $(wildcard src/tools/*/)))
TOOL_NAMES := $(patsubst src/tools/%/,%,$(TOOL_DIRS))

# build modes
ifeq ($(BUILD),debug)
	CFLAGS  := $(COMMON_CFLAGS) -O0 -g -fsanitize=address,undefined
	LDFLAGS := -fsanitize=address,undefined
else ifeq ($(BUILD),release)
	CFLAGS  := $(COMMON_CFLAGS) -O3 -DNDEBUG
	LDFLAGS := -flto
else
	$(error Unknown BUILD=$(BUILD))
endif

TOOLS_EXES := $(foreach tool,$(TOOL_NAMES),$(BIN_DIR)/$(tool)$(EXE_EXT))

.PHONY: all clean install uninstall lib sharedlib tc48emu

all: tc48emu lib sharedlib $(TOOLS_EXES)

tc48emu: $(TC48_EMU_LIB)

$(TC48_EMU_LIB):
	$(MAKE) -C $(TC48_EMU_DIR) libtc48emu FEATURES=none

lib:       $(TOBJ_LIB_STATIC)
sharedlib: $(TOBJ_LIB_SHARED)

$(TOBJ_LIB_STATIC): $(TOBJ_OBJS_STATIC)
	@$(call CMD_MKDIR_P,$(dir $@))
	$(AR) rcs $@ $^

$(TOBJ_LIB_SHARED): $(TOBJ_OBJS_SHARED)
	@$(call CMD_MKDIR_P,$(dir $@))
	$(CC) -shared $^ $(LDFLAGS) -o $@

define TOOL_RULE
TOOL_SRCS_$(1) := $$(call rwildcard,src/tools/$(1),*.c)
TOOL_OBJS_$(1) := $$(patsubst %.c,$$(OBJ_ROOT_DIR)/%.o,$$(TOOL_SRCS_$(1)))

$$(BIN_DIR)/$(1)$$(EXE_EXT): $$(TOOL_OBJS_$(1)) $$(TOBJ_LINK_DEP) $$(TC48_EMU_LIB)
	@$$(call CMD_MKDIR_P,$$(dir $$@))
	$$(CC) $$(TOOL_OBJS_$(1)) $$(TOBJ_LINK_DEP) $$(TC48_EMU_LIB) $$(LDFLAGS) -o $$@
endef

$(foreach tool,$(TOOL_NAMES),$(eval $(call TOOL_RULE,$(tool))))

ALL_C_SRCS := $(TOBJ_SRCS) $(foreach tool,$(TOOL_NAMES),$(TOOL_SRCS_$(tool)))
DEPS       := $(patsubst %.c,$(DEP_ROOT_DIR)/%.d,$(ALL_C_SRCS)) \
              $(patsubst %.c,$(DEP_ROOT_DIR)/shared/%.d,$(TOBJ_SRCS))

$(OBJ_ROOT_DIR)/%.o: %.c
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(call CMD_MKDIR_P,$(DEP_ROOT_DIR)/$(dir $<))
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_ROOT_DIR)/$*.d -c $< -o $@

$(OBJ_ROOT_DIR)/shared/%.o: %.c
	@$(call CMD_MKDIR_P,$(dir $@))
	@$(call CMD_MKDIR_P,$(DEP_ROOT_DIR)/shared/$(dir $<))
	$(CC) $(CFLAGS) $(PIC_CFLAGS) -MMD -MP -MF $(DEP_ROOT_DIR)/shared/$*.d -c $< -o $@

-include $(DEPS)

PREFIX      ?= /usr/local
BINDIR      ?= $(PREFIX)/bin
LIBDIR      ?= $(PREFIX)/lib
INCDIR      ?= $(PREFIX)/include

install: all
	$(call CMD_MKDIR_P,$(DESTDIR)$(BINDIR))
	$(call CMD_MKDIR_P,$(DESTDIR)$(LIBDIR))
	$(call CMD_MKDIR_P,$(DESTDIR)$(INCDIR))

	$(foreach tool,$(TOOL_NAMES),$(call CMD_INSTALL,755,$(BIN_DIR)/$(tool)$(EXE_EXT),$(DESTDIR)$(BINDIR)/$(tool)$(EXE_EXT));)

	$(call CMD_INSTALL,644,$(TOBJ_LIB_STATIC),$(DESTDIR)$(LIBDIR)/lib$(TOBJ_LIB_NAME)$(STATIC_EXT))
	$(call CMD_INSTALL,755,$(TOBJ_LIB_SHARED),$(DESTDIR)$(LIBDIR)/lib$(TOBJ_LIB_NAME)$(SHARED_EXT))
	$(call CMD_CP_R,include/tobj,$(DESTDIR)$(INCDIR)/)

uninstall:
	$(foreach tool,$(TOOL_NAMES),$(call CMD_RM_F,$(DESTDIR)$(BINDIR)/$(tool)$(EXE_EXT));)
	$(call CMD_RM_F,$(DESTDIR)$(LIBDIR)/lib$(TOBJ_LIB_NAME)$(STATIC_EXT))
	$(call CMD_RM_F,$(DESTDIR)$(LIBDIR)/lib$(TOBJ_LIB_NAME)$(SHARED_EXT))
	$(call CMD_RM_RF,$(DESTDIR)$(INCDIR)/tobj)

clean:
	@$(call CMD_RM_RF,build)
	@$(call CMD_RM_RF,out)
