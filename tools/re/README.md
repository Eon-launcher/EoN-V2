# Bedrock binary-analysis tools

These scripts analyze a user-supplied local library; the game binary is not
stored in this repository. Run commands from the repository root.

## Requirements

- Python 3 (standard library only for `scan_rtti.py`)
- `readelf`, `nm`, `strings`, and LLVM `llvm-objdump`

Initial resource check for this analysis: `free -h; nproc`. The host reported
7.8 GiB RAM and 2 CPUs, so Ghidra headless was intentionally not used. LIEF,
Capstone, pyelftools, radare2, and rizin were unavailable.

## Reproduce inventory

```sh
readelf -W -h -S -l -n libminecraftpe.so
nm -D --defined-only -C libminecraftpe.so
strings -a -n 7 -t x libminecraftpe.so
python3 tools/re/scan_rtti.py libminecraftpe.so
python3 tools/re/scan_rtti.py libminecraftpe.so --classes-only
python3 tools/re/scan_rtti.py libminecraftpe.so --classes-only \
  > docs/re/1.26.60.4/classes.json
```

`scan_rtti.py` reads ELF64 section headers and `.rela.dyn`, locates exact
Itanium class-name strings in `.rodata`, and follows relative relocations to
typeinfo name pointers and RTTI-backed table prefixes. A candidate is not
automatically the primary vtable; multiple tables may be secondary or
construction tables.

Use `llvm-objdump` for a bounded function disassembly. For example:

```sh
llvm-objdump --disassemble --triple=aarch64-linux-android \
  --start-address=0x6834bc0 --stop-address=0x6834c60 libminecraftpe.so
```

Use `nm -D --defined-only -C libminecraftpe.so` to reproduce the exported
symbol listing. The complete listing is intentionally not duplicated in the
Markdown report.

## Reproducing signatures

No verified feature signatures are included yet: the EoN hook source described
in the task is not present in this checkout, and the three claimed pattern
definitions could not be located. Do not manufacture replacement patterns.
When verified patterns are added to `docs/re/1.26.60.4/signatures.json`, record
the exact build ID and rerun their scanner against that same local binary.
