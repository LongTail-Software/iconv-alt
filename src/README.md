# src/ — Source Files

This directory contains the core implementation of iconv-alt.

## Files

| File | Description |
|------|-------------|
| `iconv_core.c` | Main iconv API implementation (`iconv_open`, `iconv`, `iconv_close`) |
| `sjis.c` | SJIS ↔ Unicode conversion utilities |
| `utf8.c` | UTF-8 decoding utilities |

## Public API

### iconv_core.c

| Function | Description |
|----------|-------------|
| `iconv_open(tocode, fromcode)` | Open a conversion descriptor |
| `iconv(cd, inbuf, inleft, outbuf, outleft)` | Perform character conversion |
| `iconv_close(cd)` | Close conversion descriptor |

**Supported encoding names (case-insensitive):**

| SJIS variants | UTF-8 variants |
|---------------|----------------|
| `SHIFT_JIS`, `SHIFT-JIS`, `SHIFTJIS` | `UTF-8`, `UTF8` |
| `SJIS`, `CP932`, `MS932` | `CSUTF8` |
| `Windows-31J`, `CSSHIFTJIS` | |
| `X-SJIS`, `X-MS-CP932` | |

### sjis.c

| Function | Description |
|----------|-------------|
| `sjis_to_unicode(code, *uni)` | Convert SJIS code to Unicode code point |
| `unicode_to_sjis(uni, *sjis)` | Convert Unicode code point to SJIS code |
| `sjis_to_utf8_buf(in, inlen, out, outlen)` | Bulk SJIS → UTF-8 conversion |
| `utf8_to_sjis_buf(in, inlen, out, outlen)` | Bulk UTF-8 → SJIS conversion |
| `sjis_put(code, **out, *left)` | Write SJIS byte(s) to buffer |

### utf8.c

| Function | Description |
|----------|-------------|
| `utf8_next(**p, end, *cp)` | Decode one UTF-8 character |

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    iconv_core.c                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │ iconv_open  │  │    iconv    │  │ iconv_close │     │
│  └─────────────┘  └──────┬──────┘  └─────────────┘     │
│                          │                              │
│         ┌────────────────┼────────────────┐            │
│         ▼                ▼                ▼            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │ SJIS→UTF-8  │  │ UTF-8→SJIS  │  │   Aliases   │    │
│  └──────┬──────┘  └──────┬──────┘  └─────────────┘    │
└─────────┼────────────────┼─────────────────────────────┘
          │                │
          ▼                ▼
┌─────────────────┐  ┌─────────────────┐
│     sjis.c      │  │     utf8.c      │
│ ┌─────────────┐ │  │ ┌─────────────┐ │
│ │sjis_to_unicode│ │  │ │  utf8_next  │ │
│ │unicode_to_sjis│ │  │ └─────────────┘ │
│ └─────────────┘ │  └─────────────────┘
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  sjis_table.h   │
│  (auto-generated)│
└─────────────────┘
```

## Conversion Flow

### SJIS → UTF-8

1. Read SJIS byte(s) — 1 byte (ASCII/半角カナ) or 2 bytes (full-width)
2. Look up Unicode code point via `sjis_to_unicode()` (binary search)
3. Encode as UTF-8 (1-3 bytes)
4. Write to output buffer

### UTF-8 → SJIS

1. Decode UTF-8 sequence (1-3 bytes) via `utf8_feed()`
2. Look up SJIS code via `unicode_to_sjis()` (hash-based search)
3. Write 1 or 2 bytes to output buffer

## Error Handling

All functions follow fail-fast principles:
- Invalid input → return error immediately
- No fallback substitution characters
- Set `errno` appropriately (`EILSEQ`, `EINVAL`, `E2BIG`)
