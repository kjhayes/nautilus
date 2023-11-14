ifndef __NAUT_MAKEFILE_INCLUDE__
__NAUT_MAKEFILE_INCLUDE__=1
unexport __NAUT_MAKEFILE_INCLUDE__

ifdef V
Q ?= 
else
Q ?= @
endif

ifdef Q
MAKEFLAGS += -s
define quiet-cmd =
	@printf "\t%s\t%s\n" "$(1)" "$(2)"
endef
else
define quiet-cmd =
endef
endif

define rel-dir =
$(dir $(shell realpath --relative-to $2 $1))
endef

define rel-file =
$(dir $(shell realpath --relative-to $2 $1))$(notdir $1)
endef

define abs-file =
$(shell realpath $1)
endef

endif
