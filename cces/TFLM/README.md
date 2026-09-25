# TFLM Library — SHARC-FX

This folder contains the pre-built **TensorFlow Lite for Microcontrollers (TFLM)** static library
for the SHARC-FX target, along with the CCES build project used to generate it.

## Folder structure

```
TFLM/
├── Eagle/                    ← CCES project to build libTFLM.a from source
├── Lib/                      ← Standard build (no reorder macro)
│   ├── Release/libTFLM.a
│   └── Debug/libTFLM.a
├── Lib_reordered_weights/    ← Build with USE_REORDERED_WEIGHTS_SCHEME enabled
│   ├── Release/libTFLM.a
│   └── Debug/libTFLM.a

## Which library to use

| Library folder | Macro |
|:---------------|:------|
| `Lib/` | *(none)* |
| `Lib_reordered_weights/` | `USE_REORDERED_WEIGHTS_SCHEME` |

## Rebuilding the library

The library is pre-built and ready to use. Rebuild only if you have modified the TFLM source
or kernel files.

**Standard build (outputs to `Lib/`):**

Run from the repo root:
```sh
make SHARCFX_ROOT=/c/analog/cces/<version>
```

**Reordered weights build (outputs to `Lib/`, then copy to `Lib_reordered_weights/`):**

1. Add `-DUSE_REORDERED_WEIGHTS_SCHEME` to `FLAGS` in `tools/sharcfx/lib.mk`.
2. Run `make` from the repo root.
3. Copy the output:
```sh
cp Lib/Release/libTFLM.a  Lib_reordered_weights/Release/
cp Lib/Debug/libTFLM.a    Lib_reordered_weights/Debug/
```
4. Remove `-DUSE_REORDERED_WEIGHTS_SCHEME` from `lib.mk` afterwards to restore the default.

> **⚠️ Never mix libraries:** Do not copy a library built without `USE_REORDERED_WEIGHTS_SCHEME`
> into `Lib_reordered_weights/`, or vice versa — it will produce silent incorrect inference results.
