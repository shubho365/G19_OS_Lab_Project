# =====================================================================
# Makefile for Project 2 — Custom UNIX-like Utilities
# =====================================================================
# Targets:
#   make            -> builds the shell and every custom_* utility
#   make clean      -> removes all binaries
#   make test       -> creates a sample file and runs every utility
#   make run        -> launches the shell
# =====================================================================

CC      := gcc
CFLAGS  := -Wall -Wextra -O2
SRCDIR  := src
BINDIR  := bin

UTILS   := custom_ls custom_cat custom_grep custom_wc \
           custom_cp custom_mv custom_rm
SHELL_BIN := custom_shell

ALL_BINS := $(addprefix $(BINDIR)/,$(SHELL_BIN) $(UTILS))

.PHONY: all clean test run

all: $(BINDIR) $(ALL_BINS)

$(BINDIR):
	mkdir -p $(BINDIR)

# Pattern rule: every binary in bin/ comes from src/<name>.c
$(BINDIR)/%: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(BINDIR) sample.txt sample_copy.txt sample_moved.txt

# A quick smoke test that exercises every utility once.
test: all
	@echo "---- Creating sample file ----"
	@printf "Hello world\nOperating Systems Lab\nIIT ISM Dhanbad\nxv6 is fun\n" > sample.txt
	@echo "---- custom_ls -l ----"   ; ./$(BINDIR)/custom_ls   -l .
	@echo "---- custom_cat ----"     ; ./$(BINDIR)/custom_cat  sample.txt
	@echo "---- custom_wc ----"      ; ./$(BINDIR)/custom_wc   sample.txt
	@echo "---- custom_grep ISM ----"; ./$(BINDIR)/custom_grep ISM sample.txt
	@echo "---- custom_cp ----"      ; ./$(BINDIR)/custom_cp   sample.txt sample_copy.txt
	@echo "---- custom_mv ----"      ; ./$(BINDIR)/custom_mv   sample_copy.txt sample_moved.txt
	@echo "---- custom_rm ----"      ; ./$(BINDIR)/custom_rm   sample_moved.txt
	@rm -f sample.txt
	@echo "---- All tests passed ----"

run: all
	@PATH="$$PWD/$(BINDIR):$$PATH" ./$(BINDIR)/$(SHELL_BIN)
