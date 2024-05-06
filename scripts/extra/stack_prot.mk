
ifdef NAUT_CONFIG_ENABLE_STACK_CHECK
COMMON_FLAGS += -fstack-protector-all
else
COMMON_FLAGS += -fno-stack-protector
endif

