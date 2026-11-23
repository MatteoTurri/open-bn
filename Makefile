# open-bn. Most targets run inside the open-bn-env Docker image (make env).
#
# Variables passed into the container:
#   UNITS=<file>             check only the units listed in <file> (same format as config/units.txt)
#   VERBOSE=1                print the detailed statistics of each step
#   RELINK_ALLOW_STALE=1, RELINK_ALLOW_MISSING=1   let `make link` run with stale objects or missing symbols
#   VARIANT=<name>           `make link` / `make link-cmp` build and check a variant of config/variants.txt
#   BN_TARGET=<module>       work on a module of config/modules.txt instead of bn (tools/target.py,
#                            docs/modules.md): split, symtab, check, link and link-cmp
DOCKER_ENV = $(if $(BN_TARGET),-e BN_TARGET="$(BN_TARGET)") $(if $(UNITS),-e UNITS="$(UNITS)") $(if $(VERBOSE),-e VERBOSE="$(VERBOSE)") \
    $(if $(VARIANT),-e VARIANT="$(VARIANT)") \
    $(if $(RELINK_ALLOW_STALE),-e RELINK_ALLOW_STALE="$(RELINK_ALLOW_STALE)") $(if $(RELINK_ALLOW_MISSING),-e RELINK_ALLOW_MISSING="$(RELINK_ALLOW_MISSING)")
# Run as the host user so that files written under build/ are not owned by root.
DOCKER_USER = --user $(shell id -u):$(shell id -g) -e HOME=/tmp
DOCKER = docker run --rm --platform linux/amd64 $(DOCKER_USER) $(DOCKER_ENV) -v "$(PWD):/work" open-bn-env

# A module of config/modules.txt (BN_TARGET), or empty for bn.
MODULE = $(filter-out bn,$(BN_TARGET))

# The reference disassembly (make split) is missing on a fresh clone: check runs split first.
NEED_SPLIT = $(if $(and $(wildcard expected/asm/*.s),$(wildcard expected/functions.txt),$(wildcard build/dnas/bn-jpn.dec.elf)),,1)

.PHONY: check check-bn check-module format link link-bn link-module link-in link-cmp \
        env venv need-buildx sony-gcc sony-binutils small kit headers bootstrap \
        split decrypt got-debt modules symtab scripts

## Build and verify

# Check the formatting of src/, compile every unit and compare functions, data and exception tables with the original;
# for bn also the scripts (docs/scripts.md).
check: $(if $(MODULE),check-module,check-bn)

# A module (BN_TARGET=<name>): functions, data and exception tables (docs/modules.md).
check-module:
	@[ -f orig/modules/$(MODULE).so ] || { echo "error: orig/modules/$(MODULE).so missing: run make modules" >&2; exit 2; }
	@$(if $(wildcard expected/modules/$(MODULE)/asm/*.s),,$(MAKE) --no-print-directory split)
	@$(DOCKER) python3 tools/format.py --check
	@python3 tools/check_toolchain.py
	@python3 tools/statics.py --verify-list
	@$(DOCKER) python3 tools/check.py
	@$(DOCKER) python3 tools/datacheck.py
	@$(DOCKER) python3 tools/ehcheck.py

check-bn:
	@[ -f orig/bn-jpn.elf ] || { echo "error: orig/bn-jpn.elf missing (see Building in README.md)" >&2; exit 2; }
	@$(if $(NEED_SPLIT),$(MAKE) --no-print-directory split)
	@$(DOCKER) python3 tools/format.py --check
	@python3 tools/check_toolchain.py
	@python3 tools/statics.py --verify-list
	@python3 tools/extract_assets.py
	@$(DOCKER) python3 tools/check.py
	@$(DOCKER) python3 tools/datacheck.py
	@$(DOCKER) python3 tools/ehcheck.py
	@$(if $(UNITS),true,$(DOCKER) python3 tools/scriptcheck.py)

# The scripts alone (src/bnx/, config/scripts.txt): ARGS=<path> for some, ARGS=-v for one line each.
scripts:
	@$(DOCKER) python3 tools/scriptcheck.py $(ARGS)

# Format src/ with the pinned clang-format (tools/format.py); check verifies it first.
format:
	@$(DOCKER) python3 tools/format.py

# Link build/relink/bn.elf (a module: build/modules/<name>/<name>.so) from the objects of the
# last full `make check`. fetch.py runs on the host because it needs the network (it downloads
# the third-party libraries the link needs into build/relink/dl/); the rest runs in the container.
link: $(if $(MODULE),link-module,link-bn)

link-module:
	@for f in check.json datacheck.json ehcheck.json; do \
		[ -f build/modules/$(MODULE)/$$f ] || { echo "error: build/modules/$(MODULE)/$$f missing: run a full BN_TARGET=$(MODULE) make check first" >&2; exit 2; }; \
	done
	@python3 tools/relink/fetch.py
	@$(DOCKER) sh tools/relink/link_module.sh

link-bn:
	@for f in build/check.json build/datacheck.json build/ehcheck.json; do \
		[ -f $$f ] || { echo "error: $$f missing: run a full make check first" >&2; exit 2; }; \
	done
	@mkdir -p build/relink
	@python3 tools/relink/fetch.py
	@$(DOCKER) make --no-print-directory link-in

link-in:
	@[ -f /.dockerenv ] || { echo "error: link-in runs only in the container, use make link" >&2; exit 2; }
	@python3 -c 'import elftools' 2>/dev/null || { echo "error: open-bn-env has no pyelftools, rebuild it with make env" >&2; exit 2; }
	@[ build/layoutcheck.json -nt build/check.json ] || python3 tools/layoutcheck.py > build/relink/layoutcheck.log
	@python3 tools/relink/make_libs.py > build/relink/make_libs.log
	@python3 tools/relink/link_order.py > build/relink/link_order.log
	@python3 tools/relink/make_blobs.py > build/relink/make_blobs.log
	@sh tools/relink/link.sh 2> build/relink/link.log; st=$$?; cat build/relink/link.log >&2; exit $$st

# Compare build/relink/bn.elf (a module: build/modules/<name>/<name>.so) with the original
# (exit 1 if the SHA1 differs).
link-cmp:
	@$(if $(MODULE),,[ -f build/relink/bn.elf ] || { echo "error: build/relink/bn.elf missing: run a full make check, then make link" >&2; exit 2; })
	@$(DOCKER) sh tools/relink/link_cmp.sh

## Toolchain setup

# Everything a new machine needs: the Docker image, Sony gcc and binutils, the Small compiler, the PS2 Linux Kit headers.
bootstrap: need-buildx env sony-gcc sony-binutils small kit headers
	@echo "bootstrap  done; test it with: tools/match adler32 ref/zlib-1.1.3/adler32.c"

env:
	cd docker && docker build --platform linux/amd64 -t open-bn-env .

# Host-side tools in .venv (gitignored): the pinned clang-format, used by tools/format.py on the host
# (pre-commit hook, editor). The version comes from tools/format.py.
venv:
	@[ -x .venv/bin/python ] || python3 -m venv .venv
	@.venv/bin/pip install -q --disable-pip-version-check clang-format==$$(python3 tools/format.py --version)
	@echo "venv       $$(.venv/bin/clang-format --version)"

# docker build --output (sony-gcc, sony-binutils, small) needs BuildKit: the buildx plugin.
need-buildx:
	@docker buildx version >/dev/null 2>&1 || { echo "error: docker buildx not found; install it (Ubuntu/Mint: sudo apt install docker-buildx)" >&2; exit 2; }

sony-gcc: need-buildx
	cd docker && docker build --platform linux/amd64 -f Dockerfile.sonygcc --output=toolchain/sony .

sony-binutils: need-buildx
	cd docker && docker build --platform linux/amd64 -f Dockerfile.sonybinutils --output=toolchain/sonybinutils .

# The Small 2.1.0 compiler of the scripts (docker/small/README.md) into docker/toolchain/small/sc.
small: need-buildx
	cd docker && docker build --platform linux/amd64 -f Dockerfile.small --output=toolchain/small .

# glibc and kernel headers from the PS2 Linux Kit disc 2 (archive.org) into docker/kit/rootfs.
kit:
	@python3 tools/fetch_kit.py

headers:
	@$(DOCKER) sh tools/setup_cxx_headers.sh

## Steps used by check

# Reference disassembly of every function into expected/asm/ (a module: expected/modules/<name>/asm/,
# no DNAS decryption).
split: $(if $(MODULE),,decrypt)
	@$(DOCKER) python3 tools/split.py

# Extract the original modules (config/modules.txt) into orig/modules/; MODULES="top ..." for some only.
modules:
	@python3 tools/fetch_modules.py $(MODULES)

# Regenerate the symbol list and the unnamed functions of a module from its original.
symtab:
	@[ -n "$(MODULE)" ] || { echo "error: symtab is for modules: BN_TARGET=<module> make symtab" >&2; exit 2; }
	@python3 tools/symtab.py $(MODULE) --write
	@BN_TARGET=$(MODULE) python3 tools/statics.py --write | tail -1

# Decrypt the DNAS-protected code into build/dnas/ and verify the protection pattern.
decrypt:
	@for t in dnas2_decrypt dnas1_decrypt; do \
		out=$$(python3 tools/$$t.py) || { echo "$$out" | tail -5; echo "error: tools/$$t.py failed"; exit 1; }; \
		echo "$$out"; \
	done
	@$(DOCKER) python3 tools/dnas_ref.py --verify

# Update the symbol identity debt files in config/ after a full check (ARGS=--add to add lines).
got-debt:
	@$(DOCKER) python3 tools/gotident.py --write-debt $(ARGS)
