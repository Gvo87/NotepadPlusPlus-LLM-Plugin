# Getting LLM Assistant into Notepad++ Plugins Admin

This guide takes the plugin from source to being searchable, installable, and
auto-updatable inside **Notepad++ → Plugins → Plugins Admin**.

Plugins Admin is driven by the official
[`notepad-plus-plus/nppPluginList`](https://github.com/notepad-plus-plus/nppPluginList)
repository. A plugin appears there only after a small JSON entry (pointing at a
downloadable release zip) is merged into `src/pl.x64.json`. This repo already
contains everything needed to produce that entry.

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
   will build the x64 Release DLL, run the test suite, package
   `LLMAssistant_v1.0_x64.zip` (DLL at the zip root), publish a **GitHub Release**,
   and print the **SHA-256** in both the logs and the release notes.

> No CI? Build locally with `build.bat`, then run:
>
> ```powershell
> pwsh packaging\make-release-zip.ps1 -Version 1.0
> ```
>
> It writes `dist\LLMAssistant_v1.0_x64.zip`, prints the SHA-256, and generates the
> exact JSON entry to paste.

## Step 2 — Fill in the nppPluginList entry

Use `packaging/nppPluginList-entry.json` as the template. Replace `id` with the
SHA-256 from Step 1 and make sure `repository` points at the uploaded release zip:

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

## Step 4 — Open the PR

Fork [`notepad-plus-plus/nppPluginList`](https://github.com/notepad-plus-plus/nppPluginList),
add the entry to `src/pl.x64.json` (keep the array sorted/consistent with
neighbors), commit, and open a PR that **only** modifies that JSON file. Once
merged and the signed `nppPluginList.dll` ships, everyone can search and install
LLM Assistant from Plugins Admin.

---

## How auto-update works afterwards

Plugins Admin compares the **installed DLL's binary version** with the `version`
in `pl.x64.json`. To ship an update:

1. Bump `src/Version.h`.
2. Tag `vX.Y` and push → the workflow builds and publishes a new release zip.
3. Open a follow-up PR to `nppPluginList` updating that plugin's `version`, `id`
   (new SHA-256), and `repository` URL.

Users then see the new version under the **Updates** tab.

## Notes / limits

- This plugin ships **x64 only**, so it goes in `pl.x64.json`. To also list it for
  32-bit or ARM64 Notepad++, add the corresponding build configurations to the
  project and workflow and submit entries to `pl.x86.json` / `pl.arm64.json`.
- If the plugin ever adds/removes Notepad++ API calls with version constraints,
  set `npp-compatible-versions` (and `old-versions-compatibility`) in the entry.
