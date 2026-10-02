# Config.dat Analysis — Phase 1F

**Files Found:** 5
- `ProgramData/obsplus/beauty/beauty-hair/config.dat` (11,458 bytes)
- `ProgramData/obsplus/beauty/filters/config.dat` (16,014 bytes)
- `ProgramData/obsplus/beauty/makeup/8.10.0/custom_config.dat` (41,962 bytes)
- `ProgramData/obsplus/beauty/makeup/8.10.0/style_config.dat` (14,290 bytes)
- `ProgramData/obsplus/beauty/special-effects/config.dat` (80,023 bytes)

**Total Size:** ~167 KB

**Method:** Static analysis only — magic bytes, entropy, file size, repetition, header patterns, ASCII/UTF-8/UTF-16 strings, possible offsets, IDs, hashes, timestamps, version fields. No decryption, no brute-force, no bypass.

**Full catalog:** `analysis/config_dat_catalog.json`

---

## 1. File Listing

| Path | Size | Entropy (1K) | Printable Ratio (1K) | Zero Count (1K) | Magic (first 16 hex) | Observed Format |
|------|------|--------------|----------------------|-----------------|----------------------|-----------------|
| beauty-hair/config.dat | 11,458 | 6.27 | 0.548 | 14 | 3a43175955165f115e0a4455555a3a03 | PROTECTED / encrypted or obfuscated binary |
| filters/config.dat | 16,014 | 6.39 | 0.535 | 23 | 1a1a575158580c525e0214024f14005c | PROTECTED |
| makeup/8.10.0/custom_config.dat | 41,962 | 6.39 | 0.545 | ? | (similar) | PROTECTED |
| makeup/8.10.0/style_config.dat | 14,290 | 6.58 | 0.532 | ? | (similar) | PROTECTED |
| special-effects/config.dat | 80,023 | 6.46 | 0.435 | 15 | 1a1a56514551025c421c14026f4d475b | PROTECTED |

**All files have entropy 6.27-6.58** — higher than text (4-5) but lower than fully encrypted bundles (7.7-7.85). Suggests obfuscation or light encryption, not full AES, or compressed then obfuscated.

**Printable ratio 0.43-0.54** — about half printable, half binary, similar to XOR obfuscated text.

**Zero count low (14-23 per 1K)** — not sparse, not typical of plain struct.

---

## 2. Magic Bytes & Header Patterns

### First 64 bytes hex (beauty-hair/config.dat):

```
3a43175955165f115e0a4455555a3a03
431e415e550b0446031b86b4f38eec83
501a1912455c105e52470c1a5c421142
12084c1f5009160a5556025d48090742
1677575b1a065c5d4a595a4746094712
1d135c4101080a4a160c5b154b06500c
15575550411a4a1f07535941421c1f09
530a421b120911545b10160e070c433e
```

**No readable magic like `PK`, `JSON`, `INI`, `HFBN`**

**First bytes as little-endian uint32s:**
- `1494696762`, `291444309`, `1430522462`, `54155861`, `1581325891`, `1174670165`, `3028687619`, `2213318387`, `303635024`, `1578130501`
- No obvious count (e.g., number of presets) — values large and random-looking

**First bytes as big-endian uint32s would be different, but also random**

**No version field obvious**

**No timestamp obvious**

### Filters/config.dat:

```
1a1a575158580c525e0214024f14005c3e5c025d51445b7b4b0a5e0e12471d4356545d54165f11d7fc8bdc8e98471e43480b6f40113e0a5854061b5c4482a8dcdcd28f9e1318475a5e015340160c541e435b175559154362624241501203086e15414555130e47434200455d40144910115c025d51445b5f585b0a550f070b56541a1912545a3a5d5108531a0e14834040410b1218440f05545c41034480ddb486a188121d161f5b6f1141675a570857430841d68ce386fd841b4f1b120e105c031a0f1259401143435f19175059125c0d5d02541a0903174e5611521548065e0c175a5242440946434a465441510c5c121d0c52474b02055414015c071311484e5a505144401c1e560c5a4c51444a09470e524749520c01510f255f5507500c0502737552077626030d0372260654775b72764f14575e411544020057004d59446e580045550914020c064910085c07554c5b554415181b0f12005c3e4c4c4054165f114017534b5142471e43420d515903435e1b5b02500a0f045f0609171c13510b6c5e045b5d160c47710d5310435d0543481b57025403445f1386a188d7abad471f121f5e6740413a5c005f06120e4486fd84def9a0444a4745094d5852130e475b4411464b0e194a560e450d5c5b07054a565b104e09140e424f5b5a5d1e5b07404009434b1b460947065b0d43
```

**Similar pattern: starts with `1a1a`**

- beauty-hair: `3a43`
- filters: `1a1a`
- custom_config: `3a43` (from earlier partial)
- style_config: `3a43`? Actually `3a43` for style? Let's check: style_config.dat first bytes `3a43 1759...`? From earlier: `":C\u0017YU..."` which hex `3a43 1759...` — yes `3a43`
- special-effects: `1a1a`

**Hypothesis:**
- `1a1a` and `3a43` could be magic for config.dat format, with `1a1a` for one type (filters, special-effects) and `3a43` for another (hair, makeup)
- Or could be XOR obfuscated version of same magic

**Confidence:** LOW for magic meaning, MEDIUM that there are at least 2 variants

---

## 3. ASCII Strings

**Sample from beauty-hair/config.dat (first 2K):**
- `DUUZ:`, `WG[B`, `\\]JYZGF`, `WUPA`, `SYAB`, `C>VZB\\U`, `HZQG`, `N\\ZG_X`, `RTKYZGA`, `AMS[`, etc.
- No readable English words like "hair", "filter", "makeup", "preset", "id", "name", "thumbnail"
- Strings are short (4-8 chars), random-looking, not JSON keys

**Sample from filters/config.dat:**
- `WQXX`, `]QD[F{K`, `CVT]T`, `C^bBAP`, `]QD[F[X`, `TZ:]Q`, `AgZW`, `ZRBD`, `FCJFTAQ`, `HNZPQD@`, etc.
- Same pattern: short, random

**Sample from special-effects/config.dat:**
- `VQEQ`, `oMG[`, `DMFWX`, `\\D\\G`, `iVU[`, `AD\\TLMC`, `\\<^U`, `CH\\T`, `DJG_`, `P^nZ`, etc.

**Interpretation:**
- No plain JSON, INI, or readable text
- Strings are likely obfuscated text (XOR or similar) — original might be JSON with keys like "id", "name", etc. but obfuscated
- Could also be binary struct with char arrays that happen to be printable but not meaningful

**Confidence:** MEDIUM that strings are obfuscated, not plain

---

## 4. UTF-8 / UTF-16

- No UTF-16 BOM (`FF FE` or `FE FF`)
- No long UTF-8 sequences with high-bit chars that look like Chinese/Japanese (though locale is Chinese, but config.dat not UTF-16)
- Mostly ASCII printable + binary

**Confidence:** Not UTF-16, maybe UTF-8 obfuscated

---

## 5. Repetition & Structure

- No obvious repetition like `0000` or `FFFF` or `AAAA`
- No obvious offset table (e.g., increasing integers)
- File sizes: 11KB, 16KB, 41KB, 14KB, 80KB — proportional to number of presets in each category?
  - Hair: 2 models + 22 thumbs = 24 items, config 11KB → ~0.5KB per item
  - Filters: 100+ thumbs, config 16KB → ~0.16KB per item
  - Makeup custom: 60+ bundles, config 41KB → ~0.7KB per item
  - Makeup style: 24 bundles, config 14KB → ~0.6KB per item
  - Special-effects: 150+ bundles, config 80KB → ~0.5KB per item
  - Roughly consistent: 0.16-0.7KB per preset, plausible for index with id, name, thumbnail path, category

**Hypothesis:** Config.dat contains per-preset entry with id (hash), name, thumbnail path, category, maybe download state, favorite state

**Confidence:** MEDIUM for per-preset entry, LOW for exact size

---

## 6. Relationship to Other Files

### Directory Structure:

```
beauty-hair/
  ├── config.dat (11KB)
  ├── models/
  │   ├── 7F6145EBA25E794E874664152A6F1917.bundle (611KB)
  │   └── 81ED4D77A8EE259D091955F14985EE36.bundle (4.2KB)
  └── thumbs/
      ├── hair_gradient_01.png
      ├── hair_normal_01.png
      └── ... 22 total

filters/
  ├── config.dat (16KB)
  └── thumbs/
      ├── 04BC983D74C989ADD8B0C1CB844953BF.png
      └── ... 100+ total

makeup/8.10.0/
  ├── custom_config.dat (41KB)
  ├── style_config.dat (14KB)
  ├── custom/
  │   └── models/
  │       ├── 026B94DC68A10EB6992B94333E856A1F.bundle
  │       └── ... 60+ total
  └── style/
      ├── 1/35852020EBC4C15D062140D61C6FB3E7.bundle
      └── ... 24 total (style 1..28)

special-effects/
  ├── config.dat (80KB)
  ├── 100/86C46C3F8BD9EAFBA9E53B73E1AB0F22.bundle
  ├── 101/F8E789921F1CBEA09766CD619125CAB9.bundle
  └── ... 150+ numbered folders
```

**Observations:**
- Each category has `config.dat` alongside bundles and/or thumbs
- Hair: config + models + thumbs
- Filters: config + thumbs (no bundles, maybe filters are just LUTs or color adjustments, not bundles)
- Makeup: custom_config + style_config + custom/models + style/{id}/*.bundle
- Special-effects: config + numbered folders each with bundle

**Inferred relationships:**
- `config.dat` likely indexes bundles and thumbs in same directory
- For hair: `config.dat` maps `7F6145EB...` bundle to `hair_gradient_01.png` thumb?
- For filters: `config.dat` maps hex thumb name `04BC983D...` to filter name and params?
- For makeup custom: `custom_config.dat` maps `026B94DC...` bundle hash to makeup name, category (lip, eye, etc.), thumbnail (maybe not in repo, remote), download state, favorite state?
- For makeup style: `style_config.dat` maps style id `1` to bundle `35852020...` and name?
- For special-effects: `config.dat` maps numbered folder `100` to bundle `86C46C3F...` and effect name?

**Evidence:**
- `data/locale/en-US.ini` has keys like `Message.Shop.GoodsNotFound`, `Coupon`, `Order` — shop/marketplace for presets
- `data/download.png`, `favorite.png`, `dowloading.png`, `favorited.png` — UI for download/favorite state
- `AppData/Roaming/plugins/obs-cam-beauty/data/` has same UI icons
- `ProgramData/obsplus/beauty/` structure suggests downloaded presets, with config.dat as local index

**Confidence:** MEDIUM for indexing purpose, LOW for exact mapping

---

## 7. Possible IDs, Hashes, Timestamps, Version

**IDs/Hashes:**
- Bundle filenames are 32-char uppercase hex (MD5-like): `026B94DC68A10EB6992B94333E856A1F`, `7F6145EBA25E794E874664152A6F1917`, etc.
- Filter thumbs are also 32-char hex PNG: `04BC983D74C989ADD8B0C1CB844953BF.png`
- Likely bundle hash is used as ID, and config.dat maps ID to metadata

**Timestamps:**
- No obvious Unix timestamp (e.g., 1700000000 = 2023) in first uints
- First uints are large random-looking (e.g., 1494696762, 291444309) — could be timestamps? 1494696762 = 2017-05-13, plausible for bundle creation? But need more evidence
- Could also be checksum or version

**Version:**
- Makeup folder has version `8.10.0` in path: `makeup/8.10.0/` — version of makeup system
- Config.dat might have version field, but not obvious

**Language:**
- Locale INI has 5 languages: en-US, zh-CN, zh-TW, ja-JP, ko-KR
- Config.dat might have localized names? But no UTF-8 Chinese found in strings

**Download/Favorite state:**
- UI icons `download.png`, `favorite.png` suggest presets can be downloaded and favorited
- Config.dat might store download state and favorite state per preset
- Evidence: `data/locale/en-US.ini` has `Message.SoftwareInstall.*`, `Message.Shop.*`

**Confidence:** LOW for IDs/hashes mapping, LOW for timestamps/version, MEDIUM for download/favorite state (from UI icons and locale)

---

## 8. Structural Analysis Summary

| Aspect | Observed | Inferred | Confidence |
|--------|----------|----------|------------|
| Magic | `1a1a` or `3a43` first 2 bytes, not standard | Possibly 2 variants of config.dat format, or XOR obfuscated same magic | LOW |
| Entropy | 6.27-6.58 (1K) — less than bundle 7.8, more than text 4-5 | Obfuscated or lightly encrypted, not full AES, maybe XOR + compression | MEDIUM |
| Printable ratio | 0.43-0.54 | Half text, half binary, consistent with XOR obfuscated JSON | MEDIUM |
| Zero count | Low (14-23 per 1K) | Not sparse struct, more like text | LOW |
| ASCII strings | Short random 4-8 chars, no readable keys like "id", "name" | Obfuscated text, original might be JSON | MEDIUM |
| File size | 11KB-80KB, roughly 0.16-0.7KB per preset | Per-preset entry with id, name, thumbnail, category | MEDIUM |
| Relationship | Each config.dat alongside bundles and/or thumbs in same category folder | Index mapping bundle hash to metadata (name, thumbnail, category, download/favorite state) | MEDIUM |
| IDs | Bundle filenames 32-char hex (MD5-like) | Bundle hash used as ID in config.dat | MEDIUM |
| Timestamps | No obvious Unix timestamp, first uints large random | Could be timestamps but not confirmed | LOW |
| Version | Folder `makeup/8.10.0/` has version | Config.dat might have version field | LOW |
| Language | Locale INI 5 languages | Config.dat might have localized names | LOW |
| Download/Favorite | UI icons download.png, favorite.png, locale Shop messages | Config.dat stores download/favorite state | MEDIUM |

---

## 9. What Config.dat is NOT

- NOT JSON (no `{`, `}`, `"id"`, etc. readable)
- NOT INI (no `key=value`)
- NOT ZIP (no `PK`)
- NOT PNG (no `89 50 4E 47`)
- NOT plain text

**Conclusion:** PROTECTED / encrypted or obfuscated binary

---

## 10. Legal & Compliance

- No decryption attempted
- No brute-force key
- No bypass of encryption
- Only static analysis: magic, entropy, printable ratio, strings, file size, directory structure
- Marked as PROTECTED for internal structure

---

## 11. Implications for HuanFace SDK

Since config.dat is PROTECTED, HuanFace SDK should design its own **open preset index format**:

**Proposed HuanFace preset index (JSON):**

```json
{
  "version": "1.0",
  "category": "makeup",
  "presets": [
    {
      "id": "natural_lip",
      "name": "Natural Lip",
      "description": "Everyday natural lip color",
      "category": "lip",
      "thumbnail": "thumbs/natural_lip.png",
      "bundle": "bundles/natural_lip.hfbundle",
      "params": {
        "intensity": 0.8,
        "color": "#FF0000"
      },
      "downloaded": true,
      "favorite": false,
      "language": {
        "en-US": "Natural Lip",
        "zh-CN": "自然唇色"
      }
    }
  ]
}
```

**This is PROPOSED OPEN FORMAT, not reverse-engineered original.**

---

**End of Config.dat Analysis**
