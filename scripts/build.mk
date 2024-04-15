
export

include $(SCRIPTS_DIR)/include.mk

# Default rule which will be invoked
#
# Local Makefiles can add dependencies to this rule
# to run more complicated rules
build: builtin.o


include Makefile
# obj-y should be set past here

# Empty object file to avoid complaints if a
# configuration includes a directory but no source
# files within that directory
NULL_OBJECT := $(SOURCE_DIR)/.null.o

define builtin_rule =

$(1)-subdirs := $$(filter %/,$$($(1)))
$(1)-objects := $$(filter-out %/,$$($(1)))
$(1)-cmds := $$(patsubst %.o,.%.cmd,$$($(1)-objects))
$(1)-builtins := $$(patsubst %/,%/builtin.o,$$($(1)-subdirs))

$$(foreach CMD,$$($(1)-cmds),$$(eval include $$(CMD)))

$(2): $$($(1)-objects) $$($(1)-builtins) $(NULL_OBJECT)
	$$(call quiet-cmd,LD,$$(call rel-file, $$@, $(ROOT_DIR)))
	$$(Q)$$(LD) $$(LDFLAGS) -r \
		$$(sort $$(wildcard $$($(1)-builtins)) \
		$$(wildcard $$($(1)-objects)) \
		$(NULL_OBJECT)) \
		-o $$@

endef

$(eval $(call builtin_rule,obj-y,builtin.o))

# Always recurse into subdirs
%/builtin.o: FORCE
	#$(call quiet-cmd,MAKE,$(call rel-dir, $@, $(ROOT_DIR)))
	$(Q)$(MAKE) -C $(@D) -f $(SCRIPTS_DIR)/build.mk build

%.o: %.c $(AUTOCONF)
	$(call quiet-cmd,CC,$@)
	$(Q)$(CC) $(CFLAGS) -c $(call abs-file, $<) -o $@
%.o: %.S $(AUTOCONF)
	$(call quiet-cmd,CC,$@)
	$(Q)$(CC) $(AFLAGS) -c $(call abs-file, $<) -o $@
%.o: %.cc $(AUTOCONF)
	$(call quiet-cmd,CXX,$@)
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

.%.cmd: %.c $(AUTOCONF)
	#$(call quiet-cmd,DEP,$@)
	$(Q)$(CPP) $(CPPFLAGS) -MM -MG $(call abs-file, $<) -MF $@
.%.cmd: %.S $(AUTOCONF)
	#$(call quiet-cmd,DEP,$@)
	$(Q)$(CPP) $(CPPFLAGS) -MM -MG $(call abs-file, $<) -MF $@
.%.cmd: %.cc $(AUTOCONF)
	#$(call quiet-cmd,DEP,$@)
	$(Q)$(CPP) $(CPPFLAGS) -MM -MG $(call abs-file, $<) -MF $@

FORCE:

.PHONY: builtin
