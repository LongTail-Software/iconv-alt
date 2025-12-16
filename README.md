# iconv-alt

A lightweight, clean-room implementation of the POSIX `iconv()` API for Windows.

[![CI](https://github.com/user/iconv-alt/actions/workflows/ci.yml/badge.svg)](https://github.com/user/iconv-alt/actions/workflows/ci.yml)

## Overview

**iconv-alt** provides a minimal POSIX-compatible `iconv` interface for character encoding conversion, specifically designed for Windows environments where the standard `iconv` library is not available.

### Supported Encodings

| From | To | Status |
|------|-----|--------|
| SHIFT_JIS (CP932) | UTF-8 | ✅ Supported |
| UTF-8 | SHIFT_JIS (CP932) | ✅ Supported |

Includes support for:
- Full-width characters (2-byte SJIS)
- Half-width Katakana (0xA1-0xDF → U+FF61-U+FF9F)
- ASCII (0x00-0x7F)

### Encoding Name Aliases

The following encoding names are recognized (case-insensitive):

| SJIS variants | UTF-8 variants |
|---------------|----------------|
| `SHIFT_JIS`, `SHIFT-JIS`, `SHIFTJIS` | `UTF-8`, `UTF8` |
| `SJIS`, `CP932`, `MS932` | `CSUTF8` |
| `Windows-31J`, `CSSHIFTJIS` | |
| `X-SJIS`, `X-MS-CP932` | |

## API Reference

```c
#include <iconv.h>

/* Open a conversion descriptor */
iconv_t iconv_open(const char* tocode, const char* fromcode);

/* Perform character set conversion */
size_t iconv(iconv_t cd,
             char** inbuf, size_t* inbytesleft,
             char** outbuf, size_t* outbytesleft);

/* Close a conversion descriptor */
int iconv_close(iconv_t cd);
```

### Error Handling

The `iconv()` function returns `(size_t)-1` on error and sets `errno`:

| errno | Meaning |
|-------|---------|
| `EILSEQ` | Invalid or unconvertible character sequence |
| `EINVAL` | Incomplete multibyte sequence at end of input |
| `E2BIG` | Output buffer is too small |

## Usage Example

```c
#include <iconv.h>
#include <stdio.h>
#include <string.h>

int main() {
    /* SJIS "あいう" */
    const char sjis[] = "\x82\xa0\x82\xa2\x82\xa4";
    char utf8[32] = {0};
    
    char* in = (char*)sjis;
    char* out = utf8;
    size_t inleft = sizeof(sjis) - 1;
    size_t outleft = sizeof(utf8);
    
    /* Open converter: SHIFT_JIS -> UTF-8 */
    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    if (cd == (iconv_t)-1) {
        perror("iconv_open");
        return 1;
    }
    
    /* Convert */
    if (iconv(cd, &in, &inleft, &out, &outleft) == (size_t)-1) {
        perror("iconv");
        iconv_close(cd);
        return 1;
    }
    
    printf("UTF-8: %s\n", utf8);  /* Output: あいう */
    
    iconv_close(cd);
    return 0;
}
```

## Building

### Requirements

- CMake 3.21+
- C17 compiler (MSVC recommended for Windows)
- Python 3.x (for code generation)
- [vcpkg](https://github.com/microsoft/vcpkg) (for Google Test)

### Windows (Visual Studio 2022)

```powershell
# Clone vcpkg (if not already installed)
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# Configure and build
cmake --preset windows-msvc-release `
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build --preset windows-msvc-release-build

# Run tests
ctest --preset windows-msvc-release-test
```

### Available Presets

| Configure Preset | Build Preset | Description |
|-----------------|--------------|-------------|
| `windows-msvc-release` | `windows-msvc-release-build` | Release build with MSVC |
| `windows-msvc-debug` | `windows-msvc-debug-build` | Debug build with MSVC |
| `vs2022-release` | `vs2022-release-build` | Visual Studio 2022 solution |

## Project Structure

```
iconv-alt/
├── include/
│   ├── iconv.h          # Public API header
│   └── sjis_table.h     # Auto-generated SJIS↔Unicode mapping
├── src/
│   ├── iconv_core.c     # iconv_open/iconv/iconv_close implementation
│   ├── sjis.c           # SJIS conversion utilities
│   └── utf8.c           # UTF-8 decoding utilities
├── scripts/
│   ├── gen_sjis_table.py  # Generates sjis_table.h from CP932.TXT
│   └── gen_cases.py       # Generates comprehensive test cases
├── tests/
│   ├── smoke.cpp        # Build verification test
│   ├── sjis_utf8.cpp    # Round-trip and error tests
│   └── auto_rt.cpp      # Auto-generated exhaustive tests (Debug only)
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

## Code Generation

The SJIS↔Unicode mapping table is automatically generated from the official [CP932.TXT](https://ftp.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP932.TXT) file:

```bash
python scripts/gen_sjis_table.py   # Generates include/sjis_table.h
python scripts/gen_cases.py        # Generates tests/auto_rt.cpp
```

## Tests

| Test | Description |
|------|-------------|
| `smoke` | Basic build verification |
| `RoundTrip.Basic` | Full-width character round-trip (あいう) |
| `RoundTrip.HalfWidthKatakana` | Half-width katakana round-trip (ｱｲｳｴｵ) |
| `RoundTrip.Ascii` | ASCII round-trip |
| `RoundTrip.Mixed` | Mixed encoding round-trip |
| `Error.Utf8ToSjis_IllegalSequence` | Unconvertible character (emoji) |
| `Error.Utf8ToSjis_IncompleteSequence` | Truncated UTF-8 input |
| `Error.SjisToUtf8_BufferTooSmall` | Output buffer overflow |
| `Alias.CP932ToUtf8` | CP932 encoding name alias |
| `Alias.Utf8ToSjis` | Case-insensitive encoding names |
| `Alias.Windows31J` | Windows-31J encoding name alias |
| `Alias.InvalidEncoding` | Unknown encoding returns error |

## License

Apache License 2.0 - See [LICENSE](LICENSE) for details.

## Technical Notes

### Why iconv-alt?

Windows does not include a standard `iconv` implementation. While alternatives exist (GNU libiconv, win-iconv), they may:
- Require LGPL/GPL compliance
- Have complex build dependencies
- Include more functionality than needed

**iconv-alt** provides:
- ✅ Clean-room Apache 2.0 implementation
- ✅ Minimal footprint (SJIS ↔ UTF-8 only)
- ✅ No external dependencies at runtime
- ✅ Simple CMake integration

### Character Encoding Details

The implementation uses the official Microsoft CP932 mapping, which is a superset of JIS X 0208. It includes:

- **JIS X 0201**: ASCII + Half-width Katakana
- **JIS X 0208**: Full-width characters (Kanji, Hiragana, Katakana, symbols)
- **NEC/IBM extensions**: Additional vendor-specific characters
