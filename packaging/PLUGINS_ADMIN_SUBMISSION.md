# Getting LLM Assistant into Notepad++ Plugins Admin

This guide takes the plugin from source to being searchable, installable, and
auto-updatable inside **Notepad++ → Plugins → Plugins Admin**.

Plugins Admin is driven by the official
[`notepad-plus-plus/nppPluginList`](https://github.com/notepad-plus-plus/nppPluginList)
repository. A plugin appears there only after a small JSON entry (pointing at a
downloadable release zip) is merged into the list for that Notepad++
architecture: `src/pl.x64.json`, `src/pl.x86.json` (32-bit) or
`src/pl.arm64.json`. Each list needs its own zip, because Notepad++ only loads a
DLL built for its own architecture. This repo already contains everything needed
to produce those entries.

---

## What Plugins Admin requires

| Requirement | How this repo satisfies it |
| --- | --- |
| DLL carries a real binary version (for update detection) | `src/Version.h` + `VERSIONINFO` in `src/LLMAssistant.rc` |
| A downloadable `.zip` with the DLL **at the zip root**, named exactly `LLMAssistant.dll` | `packaging/make-release-zip.ps1` and the release workflow |
| SHA-256 of that zip (used as `id`) | printed by the script and the workflow |
| `folder-name` == DLL name (`LLMAssistant`) | enforced in the JSON entry |
| A stable download URL | GitHub Release asset created by the workflow |

---

## Step 1 — Cut a versioned release of the plugin

1. Set the version in `src/Version.h` (e.g. `1.0`) — this is the single source of
   truth and gets compiled into the DLL.
2. Commit, then tag and push:

   ```bash
   git tag v1.0
   git push origin v1.0
   ```

3. The [`build-and-release`](../.github/workflows/build-and-release.yml) workflow
   will build the x64, Win32 (x86) and ARM64 Release DLLs, run the test suite,
   package `LLMAssistant_v1.0_x64.zip`, `LLMAssistant_v1.0_x86.zip` and
   `LLMAssistant_v1.0_arm64.zip` (DLL at the zip root), publish a
   **GitHub Release** with all three, and print each zip's **SHA-256** in both
   the logs and the release notes.

> No CI? Build locally with `build.bat` (or `build.bat x86`, `build.bat arm64`),
> then run:
>
> ```powershell
> pwsh packaging\make-release-zip.ps1 -Version 1.0
> pwsh packaging\make-release-zip.ps1 -Version 1.0 -Platform x86
> pwsh packaging\make-release-zip.ps1 -Version 1.0 -Platform arm64
> ```
>
> Each run checks that the DLL is built for that architecture, carries version
> 1.0 and exports the Notepad++ plugin functions, then writes
> `dist\LLMAssistant_v1.0_<platform>.zip`, prints the SHA-256, and generates the
> exact JSON entry to paste.

## Step 2 — Fill in the nppPluginList entry

Use `packaging/nppPluginList-entry.json` as the template. Replace `id` with the
SHA-256 from Step 1 and make sure `repository` points at the uploaded release zip.
The x86 and arm64 entries are the same apart from `id` and `repository`, which
point at that architecture's zip:

```json
{
  "folder-name": "LLMAssistant",
  "display-name": "LLM Assistant",
  "version": "1.0",
  "id": "<SHA-256 OF THE RELEASE ZIP>",
  "repository": "https://github.com/Gvo87/NotepadPlusPlus-LLM-Plugin/releases/download/v1.0/LLMAssistant_v1.0_x64.zip",
  "description": "AI-powered text processing (enhance, translate, summarize, grammar, code) via the OpenAI API, directly inside Notepad++.",
  "author": "Gvo87",
  "homepage": "https://github.com/Gvo87/NotepadPlusPlus-LLM-Plugin"
}
```

## Step 3 — Test locally before the PR (required)

1. Download a **debug** build of Notepad++ (x64) — see the
   [official instructions](https://npp-user-manual.org/docs/plugins/#test-your-plugins-locally).
2. Grab `src/pl.x64.json` from your fork of `nppPluginList` and save it to
   `<npp>\plugins\Config\nppPluginList.json`.
3. Add your entry to the `npp-plugins` array in that file.
4. Launch `notepad++.x64.dbg.exe` and open **Plugins → Plugins Admin**.
5. Confirm the plugin shows under **Available**, installs correctly, and appears
   under **Installed**.

For the x86 and arm64 entries, do the same with that architecture's debug build
of Notepad++ and its list (`src/pl.x86.json`, `src/pl.arm64.json`).

## Step 4 — Open the PR

Fork [`notepad-plus-plus/nppPluginList`](https://github.com/notepad-plus-plus/nppPluginList),
add the entry to `src/pl.x64.json`, `src/pl.x86.json` and `src/pl.arm64.json`
(keep each array sorted/consistent with neighbors), commit, and open a PR that
**only** modifies those JSON files. Once
merged and the signed `nppPluginList.dll` ships, everyone can search and install
LLM Assistant from Plugins Admin.

---

## How auto-update works afterwards

Plugins Admin compares the **installed DLL's binary version** with the `version`
in its architecture's list (`pl.x64.json`, `pl.x86.json` or `pl.arm64.json`).
To ship an update:

1. Bump `src/Version.h`.
2. Tag `vX.Y` and push → the workflow builds and publishes the new release zips.
3. Open a follow-up PR to `nppPluginList` updating that plugin's `version`, `id`
   (new SHA-256), and `repository` URL in each of the three lists.

Users then see the new version under the **Updates** tab.

## Notes / limits

- Each list must point at the zip for its own architecture. The workflow and
  `make-release-zip.ps1` refuse to package a DLL whose CPU type doesn't match
  the zip it goes in.
- If the plugin ever adds/removes Notepad++ API calls with version constraints,
  set `npp-compatible-versions` (and `old-versions-compatibility`) in the entry.
