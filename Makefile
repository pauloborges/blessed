# blessed Makefile

-include Makefile.config

ifeq ($(PLATFORM),)
        $(error PLATFORM is not defined)
endif

PLATFORM_PATH		= platform/$(PLATFORM)
-include $(PLATFORM_PATH)/Makefile.platform

CONFIGS			= $(addprefix -D, $(-*-command-variables-*-))

BUILD_PATH		= build
LIB_TARGET		= $(BUILD_PATH)/libblessed.a

INCLUDE_PATHS		= $(PLATFORM_INCLUDE_PATHS)			\
			  include					\
			  stack

INCLUDES		= $(addprefix -I, $(INCLUDE_PATHS))

CFLAGS			= -O2 -Wall $(PLATFORM_CFLAGS)			\
			  $(INCLUDES)					\
			  $(CONFIGS)

ASMFLAGS		= -O2 -Wall $(PLATFORM_ASMFLAGS)

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
	@echo -e "AR\t $@"
	@$(AR) $(ARFLAGS) $@ $(C_OBJ_FILES) $(ASM_OBJ_FILES) > /dev/null 2>&1
	@echo -e "SIZE\t $@"
	@$(SIZE) $@

$(BUILD_PATH)/%.o: %.c
	@echo -e "CC\t $<"
	@$(CC) $(CFLAGS) -o $@ $<

$(BUILD_PATH)/%.o: %.s
	@echo -e "CC\t $<"
	@$(CC) $(ASMFLAGS) -o $@ $<

$(BUILD_PATH):
	@echo -e "MKDIR\t $@"
	@-mkdir $@

clean:
	@echo -e "CLEAN"
	@rm -rf $(BUILD_PATH) *.log

maintainer-clean: clean
	$(MAKE) -C examples clean

examples: all
	$(MAKE) -C examples

.PHONY: all clean examples
