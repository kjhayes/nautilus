
CFLAGS += \
	  -mno-outline-atomics \
	  -mstrict-align \
	  -Werror=pointer-sign \
	  -fomit-frame-pointer \
	  -Werror-implicit-function-declaration

LDFLAGS += --fatal-warnings

