# Executable Analysis — Phase 1L

**Executables Found:** 11
- `bin/64bit/beauty.exe` (1.4 MB) + `bin/32bit/beauty.exe` (1.3 MB)
- `bin/64bit/camera-tool.exe` (176 KB) + `bin/32bit/camera-tool.exe` (138 KB)
- `AppData/Roaming/store/64bit/obs-plugin-console.exe` + `32bit` (store console)
- `ProgramData/obs-studio/plugins/win-spout/uninstall-spout2-plugin.exe`
- Plus duplicates in `AppData/.../bin/`

**Method:** Non-invasive static observation only — strings, PE imports, manifest, no execution (no Wine available), no bypass, no memory dump.

**Full observations:** `analysis/executable_observations.json`

---

## 1. beauty.exe

### 1.1 File Info

- **Path:** `bin/64bit/beauty.exe` (1.4 MB), `bin/32bit/beauty.exe` (1.3 MB)
- **Type:** Windows PE32+ (64-bit) / PE32 (32-bit)
- **Build PDB:** `D:\work\obsplus\client\obs-studio\build\x64\plugins\obsplus\obs-cam-beauty\nama\RelWithDebInfo\beauty.pdb` — built with OBS Studio build system, RelWithDebInfo config
- **Imports:** `CRYPT32.dll`, `WS2_32.dll`, `bcrypt.dll`, `VERSION.dll`, `ole32.dll`, `SHELL32.dll`, `KERNEL32.dll`, `w32-pthreads.dll`, `USER32.dll`, `GDI32.dll`, `ADVAPI32.dll`, `OPENGL32.dll`, `CNamaSDK.dll`

### 1.2 Strings Analysis

**No --help, -h, /?, usage, version strings found** — suggests GUI app, not CLI

**Relevant strings:**
- `VERSION.dll`, `GetFileVersionInfoSizeW` — version info
- `curl_easy_init`, `curl_global_init`, `curl_mime_init`, `curl_multi_init`, `curl_share_init`, `curl_easy_strerror`, `curl_multi_strerror`, `curl_share_strerror`, `curl_url_strerror` — uses libcurl for HTTP (maybe for downloading presets or auth)
- `fuGetSystemError` — FaceUnity error handling
- `CNamaSDK.dll` — loads FaceUnity SDK
- `beauty.exe` — self name
- XML manifest: `<?xml version='1.0' encoding='UTF-8' standalone='yes'?><assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>` — standard Windows manifest

**No obvious CLI args like `--input`, `--output`, `--bundle`, etc.**

### 1.3 Hypothesized Purpose

From name `beauty.exe`, imports `CNamaSDK.dll` + `OPENGL32.dll`, build path `.../obs-cam-beauty/nama/RelWithDebInfo/`:

- Standalone beauty test harness or helper
- Maybe used for testing FaceUnity SDK without OBS
- Could be used for `fuImageBeauty*` APIs (image-based beauty, not video)
- From `API_REFERENCE.md`: `fuImageBeautyCreateTexture`, `fuImageBeautySetParam`, `fuImageBeautyPreProcess`, `fuImageBeautyPreview`, `fuImageBeautyGetResult`, etc. — image beauty (process single image, not video)
- `beauty.exe` might be for `fuImageBeauty` testing

**Evidence:**
- Name `beauty.exe`
- Imports `CNamaSDK.dll` and `OPENGL32.dll`
- Build path in `nama` folder (FaceUnity NamaSDK)
- `fuImageBeauty*` APIs exist in CNamaSDK.dll

**Confidence:** MEDIUM for purpose (image beauty test harness)

### 1.4 Observed Behavior (Attempted)

**Command attempted:** `beauty.exe --help`, `beauty.exe /?`, `beauty.exe -h`, `beauty.exe --version`

**Result:** Cannot execute — no Wine available in sandbox, file is Windows PE, Linux cannot run directly

**Method:** Tried `which wine` — not found, `wine --version` — not found

**Compliance:** Did not attempt to install Wine or bypass, marked as PROTECTED for dynamic behavior

**Output:**

```json
{
  "executable": "bin/64bit/beauty.exe",
  "command": "beauty.exe --help",
  "result": "NOT EXECUTED — no Wine, Windows PE cannot run on Linux",
  "stdout": null,
  "stderr": null,
  "exit_code": null,
  "observed_behavior": "Static only: GUI app likely, no CLI help strings, uses libcurl, CNamaSDK, OpenGL",
  "imports": ["CRYPT32.dll", "WS2_32.dll", "bcrypt.dll", "VERSION.dll", "ole32.dll", "SHELL32.dll", "KERNEL32.dll", "w32-pthreads.dll", "USER32.dll", "GDI32.dll", "ADVAPI32.dll", "OPENGL32.dll", "CNamaSDK.dll"],
  "strings_of_interest": ["beauty.exe", "CNamaSDK.dll", "curl_easy_init", "fuGetSystemError"],
  "confidence": "MEDIUM for purpose, LOW for dynamic behavior",
  "status": "PROTECTED / NOT ANALYZED for dynamic behavior (requires Windows or Wine, no bypass)"
}
```

**Status:** PROTECTED / NOT ANALYZED for dynamic behavior

---

## 2. camera-tool.exe

### 2.1 File Info

- **Path:** `bin/64bit/camera-tool.exe` (176 KB), `bin/32bit/camera-tool.exe` (138 KB)
- **Type:** Windows PE
- **Imports:** `SetupDiEnumDeviceInfo`, `SetupDiDestroyDeviceInfoList`, `SetupDiGetDevicePropertyW`, `SetupDiGetDeviceRegistryPropertyW`, `QueryDosDeviceW`, `GetLastError`, `SetLastError`, `InitializeCriticalSectionEx`, `RoInitialize`, `RoUninitialize`
- **Manifest:** `<requestedExecutionLevel level='requireAdministrator' uiAccess='false' />` — requires admin

### 2.2 Strings Analysis

- No --help
- `map/set too long`, `MM/dd/yy`, ` !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~` — standard C++ strings
- `SetupDi*` — device enumeration API
- `QueryDosDeviceW` — query DOS device (camera device)
- `requireAdministrator` — requires admin

### 2.3 Hypothesized Purpose

From name `camera-tool.exe`, imports `SetupDi*`, manifest requires admin, locale INI:

- `CheckWhereDeviceUsed.ButtonText="Check Usage"`
- `CheckWhereDeviceUsed.Info="Checking camera device usage, please be patient......"`
- `CheckWhereDeviceUsed.DlgTitle="Camera Occupation Detection Tool"`
- `CheckWhereDeviceUsed.CheckError="Unable to detect camera status.\n\nCommon causes include:Wrong physical camera selected,Missing drivers,Poor connection.\nPlease check and resolve the issue."`
- `CheckWhereDeviceUsed.OpenDir="Locate File"`
- `CheckWhereDeviceUsed.NotUseed="No device occupation detected, please directly activate usage by opening the beauty source properties window"`
- `CheckWhereDeviceUsed.FindCameraSourceInOBS="The camera device is occupied by another source, please check on your own"`
- `CheckWhereDeviceUsed.FindCameraSourceInOBS.FindList="The camera device is occupied by the following sources:"`

**Purpose:** Camera occupation detection tool — helps user find which app or OBS source is using camera device, when beauty camera fails to activate because device is busy

**Evidence:**
- Name `camera-tool.exe`
- Imports device enumeration APIs
- Requires admin (to query device usage)
- Locale INI describes camera occupation detection tool

**Confidence:** HIGH for purpose

### 2.4 Observed Behavior (Attempted)

**Command attempted:** `camera-tool.exe --help`, `camera-tool.exe /?`, etc.

**Result:** Cannot execute — no Wine, requires admin, Windows PE

**Output:**

```json
{
  "executable": "bin/64bit/camera-tool.exe",
  "command": "camera-tool.exe --help",
  "result": "NOT EXECUTED — no Wine, requires admin, Windows PE",
  "stdout": null,
  "stderr": null,
  "exit_code": null,
  "observed_behavior": "Static only: device enumeration tool, requires admin, for checking camera occupation",
  "imports": ["SetupDiEnumDeviceInfo", "SetupDiDestroyDeviceInfoList", "SetupDiGetDevicePropertyW", "SetupDiGetDeviceRegistryPropertyW", "QueryDosDeviceW"],
  "manifest": "requireAdministrator",
  "locale_evidence": "CheckWhereDeviceUsed.DlgTitle=Camera Occupation Detection Tool",
  "confidence": "HIGH for purpose, LOW for dynamic behavior",
  "status": "PROTECTED / NOT ANALYZED for dynamic behavior"
}
```

**Status:** PROTECTED / NOT ANALYZED for dynamic behavior

---

## 3. obs-plugin-console.exe

### 3.1 File Info

- **Path:** `AppData/Roaming/store/64bit/obs-plugin-console.exe` + `32bit`
- **Size:** Small (few hundred KB)
- **Imports:** Likely `obs-plugins-store.dll`, `store_qt5/6.dll`, `websocket.dll`, `core.dll`

### 3.2 Hypothesized Purpose

- Console helper for OBS Store marketplace
- Handles plugin download, auth, proxy, etc.

**Evidence:**
- Path in `store/` folder
- Alongside `core.dll`, `obs-plugins-store.dll`, `websocket.dll`
- Store locale INI

**Confidence:** MEDIUM

### 3.3 Observed Behavior

**Result:** NOT EXECUTED — Windows PE, no Wine

**Status:** PROTECTED / NOT ANALYZED

---

## 4. uninstall-spout2-plugin.exe

- **Path:** `ProgramData/obs-studio/plugins/win-spout/uninstall-spout2-plugin.exe`
- **Purpose:** Uninstaller for Spout2 plugin
- **Confidence:** HIGH (name)

**Status:** Not analyzed (uninstaller, not relevant to beauty)

---

## 5. Summary

| Executable | Size | Imports | Purpose | CLI Help? | Dynamic Behavior | Confidence | Status |
|------------|------|---------|---------|-----------|------------------|------------|--------|
| beauty.exe (64/32) | 1.4/1.3 MB | CNamaSDK.dll, OPENGL32, CRYPT32, WS2_32, libcurl | Standalone beauty test harness for fuImageBeauty (image-based beauty) | No --help strings, likely GUI | NOT EXECUTED (no Wine) | MEDIUM for purpose, LOW for dynamic | PROTECTED / NOT ANALYZED |
| camera-tool.exe (64/32) | 176/138 KB | SetupDi*, QueryDosDeviceW, requires admin | Camera occupation detection tool (check which app uses camera) | No --help, GUI | NOT EXECUTED (no Wine, requires admin) | HIGH for purpose, LOW for dynamic | PROTECTED / NOT ANALYZED |
| obs-plugin-console.exe | Small | store DLLs | OBS Store console helper | Unknown | NOT EXECUTED | MEDIUM | PROTECTED |
| uninstall-spout2-plugin.exe | Small | - | Uninstaller for Spout2 | Unknown | NOT ANALYZED (not relevant) | HIGH | NOT ANALYZED |

**Total executables:** 11 (including duplicates)

**All marked as PROTECTED for dynamic behavior because they require Windows/Wine and/or admin, and we do not bypass.**

**No execution attempted beyond static strings and import table.**

---

**End of Executable Analysis**
