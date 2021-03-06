BIN_DIR 	:= build/
SRC_DIR 	:= ../analysis/
LOG_DIR 	:= log/
FIG_DIR 	:= fig/
FIGURES 	:= $(shell ls $(FIG_DIR)*.dot)
RECIPE_DIR 	:= recipe/
RECIPES 	:= SIG_tea.xml SIG_sha1.xml SIG_rc4.xml SIG_md5.xml SIG_hmac.xml SIG_aes.xml SIG_cbc.xml SIG_ctr.xml
EXE 		:= analysis

define TRACE_template
trace_$(1): $$(RECIPE_DIR)$(1)
	@ if [ ! -d $$(LOG_DIR) ]; then mkdir -p $$(LOG_DIR); fi
	./test.py $$^ TRACE
endef

define SEARCH_template
search_$(1): $$(RECIPE_DIR)$(1)
	@ if [ ! -d $$(LOG_DIR) ]; then mkdir -p $$(LOG_DIR); fi
	./test.py $$^ SEARCH
endef

all:
	@ if [ ! -d $(BIN_DIR) ]; then mkdir -p $(BIN_DIR); fi
	@ if [ ! -d $(FIG_DIR) ]; then mkdir -p $(FIG_DIR); fi
	@ cd $(SRC_DIR) && $(MAKE) VERBOSE=0 DEBUG=0 BUILD_DIR=../test/$(BIN_DIR) IOREL=1

trace: $(foreach recipe,$(RECIPES),trace_$(recipe))

$(foreach recipe,$(RECIPES),$(eval $(call TRACE_template,$(recipe))))

search: all $(foreach recipe,$(RECIPES),search_$(recipe))

$(foreach recipe,$(RECIPES),$(eval $(call SEARCH_template,$(recipe))))

figure: $(FIGURES:dot=png)

$(FIG_DIR)%.png: $(FIG_DIR)%.dot
	dot -Tpng -o $@ $^

clean:
	@ rm -rf $(BIN_DIR)
	@ rm -f $(EXE)

cleaner: clean
	@ rm -rf trace*
	@ rm -rf $(LOG_DIR)
	@ rm -f *.pyc
	@ rm -rf $(FIG_DIR)
