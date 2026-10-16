#ifndef LLMASSISTANT_VERSION_H
#define LLMASSISTANT_VERSION_H

// Single source of truth for the plugin version.
//
// This version is compiled into the DLL's VERSIONINFO resource. Notepad++'s
// Plugins Admin reads that binary version and compares it against the "version"
// field of the entry in nppPluginList (pl.x64.json) to decide whether an update
// is available. When you cut a new release, bump the numbers here, tag the repo
// with a matching "vMAJOR.MINOR" tag, and update the nppPluginList entry so
// auto-update keeps working.

#define LLM_VERSION_MAJOR 1
#define LLM_VERSION_MINOR 1
#define LLM_VERSION_PATCH 0
#define LLM_VERSION_BUILD 0

// Comma form used by FILEVERSION / PRODUCTVERSION.
#define LLM_VERSION_COMMA LLM_VERSION_MAJOR, LLM_VERSION_MINOR, LLM_VERSION_PATCH, LLM_VERSION_BUILD

// Helpers to stringize the dotted version.
#define LLM_STR_HELPER(x) #x
#define LLM_STR(x) LLM_STR_HELPER(x)

// Full "1.0.0.0" string for the version resource.
#define LLM_VERSION_STRING \
    LLM_STR(LLM_VERSION_MAJOR) "." LLM_STR(LLM_VERSION_MINOR) "." \
    LLM_STR(LLM_VERSION_PATCH) "." LLM_STR(LLM_VERSION_BUILD)

// Short "1.0" string that matches the nppPluginList "version" field.
#define LLM_VERSION_SHORT LLM_STR(LLM_VERSION_MAJOR) "." LLM_STR(LLM_VERSION_MINOR)

#endif // LLMASSISTANT_VERSION_H
