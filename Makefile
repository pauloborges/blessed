# BLEStack Makefile

-include Makefile.config

ifeq ($(PLATFORM),)
        $(error PLATFORM is not defined)
endif

PLATFORM_PATH		= platform/$(PLATFORM)
-include $(PLATFORM_PATH)/Makefile.platform

BUILD_PATH		= build
LIB_TARGET		= $(BUILD_PATH)/libblestack.a

INCLUDE_PATHS		= $(PLATFORM_INCLUDE_PATHS)			\
			  stack

INCLUDES		= $(addprefix -I, $(INCLUDE_PATHS))

CFLAGS			= $(PLATFORM_CFLAGS)				\
			  $(INCLUDES)

ASMFLAGS		= $(PLATFORM_ASMFLAGS)

LDFLAGS			= $(PLATFORM_LDFLAGS)

SOURCE_PATHS		= $(PLATFORM_SOURCE_PATHS)			\
			  stack

SOURCE_FILES		= $(PLATFORM_SOURCE_FILES)			\
			  ll.c						\
			  bci.c

ASM_PATHS		= $(PLATFORM_ASM_PATHS)

ASM_FILES		= $(PLATFORM_ASM_FILES)

C_OBJ_FILES		= $(addprefix $(BUILD_PATH)/, $(SOURCE_FILES:.c=.o))
ASM_OBJ_FILES		= $(addprefix $(BUILD_PATH)/, $(ASM_FILES:.s=.o))

vpath %.c $(SOURCE_PATHS)
vpath %.s $(ASM_PATHS)

all: $(BUILD_PATH) $(LIB_TARGET)

$(LIB_TARGET): $(C_OBJ_FILES) $(ASM_OBJ_FILES)
	@echo "  AR\t$@"
	@$(AR) $(ARFLAGS) $@ $(C_OBJ_FILES) $(ASM_OBJ_FILES) > /dev/null 2>&1

$(BUILD_PATH)/%.o: %.c
	@echo "  CC\t$<"
	@$(CC) $(CFLAGS) -o $@ $<

$(BUILD_PATH)/%.o: %.s
	@echo "  CC\t$<"
	@$(CC) $(ASMFLAGS) -o $@ $<

$(BUILD_PATH):
	@echo " MKDIR\t$@"
	@-mkdir $@

clean:
	@echo " CLEAN"
	@rm -rf $(BUILD_PATH) *.log

.PHONY: all clean
