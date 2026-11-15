#!/bin/sh
# Body of `make link-cmp` (docs/linking.md): per-section comparison (tools/relink/cmp.py,
# report also in build/relink/cmp.txt) and SHA1 against the original.
# Exit: 0 identical SHA1, 1 different SHA1 (the measure of the distance from the goal), 2 cmp.py
# broken (cmp.py exits 0/1 on the verdict, 2 on an exception) or a missing file. Runs in the
# container (make link-cmp launches it with $(DOCKER) and make exits with its code: `Error 1` /
# `Error 2`).
# pyelftools before cmp.py: an ImportError at load time exits 1 (outside cmp.py's try) and would
# pass for the "SHA1 differs" verdict instead of a broken comparison (old image: `make env`)
python3 -c 'import elftools' 2>/dev/null || { echo "error: open-bn-env image without pyelftools: rerun make env" >&2; exit 2; }
[ -f orig/bn-jpn.elf ] || { echo "error: orig/bn-jpn.elf missing" >&2; exit 2; }
[ -f build/relink/bn.elf ] || { echo "error: build/relink/bn.elf missing: run make link" >&2; exit 2; }
# Output: identical SHA1 -> one line; otherwise the differing sections (cmp.py's stderr, at most
# 20) and a line with the two SHA1s. The full report is always in build/relink/cmp.txt; VERBOSE=1
# prints it too.
# VARIANT=<name>: the expected SHA1 is the variant's (config/variants.txt); bn.elf must come from
# `make link` with the same VARIANT (build/relink/variant.txt). The section report still compares
# with the original, so for a variant it lists the sections the variant changes.
built=$(cat build/relink/variant.txt 2>/dev/null || true)
if [ "$built" != "${VARIANT:-}" ]; then
    echo "error: build/relink/bn.elf was linked with VARIANT='$built', not '${VARIANT:-}': rerun make link" >&2
    exit 2
fi
# with a variant cmp.py's stderr (the sections differing from the original) is expected: report only
rm -f build/relink/cmp.err
if [ -n "${VARIANT:-}" ]; then
    python3 tools/relink/cmp.py build/relink/bn.elf > build/relink/cmp.txt 2> build/relink/cmp.err; st=$?
else
    python3 tools/relink/cmp.py build/relink/bn.elf > build/relink/cmp.txt; st=$?
fi
[ -n "${VERBOSE:-}" ] && cat build/relink/cmp.txt
if [ $st -gt 1 ]; then cat build/relink/cmp.err >&2 2>/dev/null; echo "error: cmp.py failed (exit $st)" >&2; exit 2; fi
if [ -n "${VARIANT:-}" ]; then
    o=$(python3 tools/relink/variant.py --sha1 "$VARIANT") || exit 2
else
    o=$(sha1sum orig/bn-jpn.elf | cut -d' ' -f1)
fi
l=$(sha1sum build/relink/bn.elf | cut -d' ' -f1)
if [ "$o" = "$l" ]; then echo "SHA1 $l identical${VARIANT:+ (variant $VARIANT)}"; exit 0; fi
[ $st -eq 0 ] && [ -z "${VARIANT:-}" ] && echo "all sections identical: the difference is in the ELF/program headers or the file layout" >&2
echo "SHA1 differs: ${VARIANT:-orig} $o, linked $l (report against the original: build/relink/cmp.txt)"
exit 1
