# Notepad++ LLM Assistant Plugin

A powerful Notepad++ plugin that integrates Large Language Model (LLM) capabilities using the OpenAI API, bringing AI-powered text processing directly into your favorite text editor.

## 🚀 Features

### Core LLM Functions
- **🎯 Custom Prompt**: Enter any custom prompt for flexible AI interactions
- **✨ Enhance Text**: Improve and refine your text while maintaining its original meaning
- **💻 Generate Code**: Generate code based on descriptions or comments
- **📝 Complete Text**: Complete partial text naturally and contextually
- **📖 Check Grammar**: Check and correct grammar in your text
- **🌍 Translate Text**: Translate text to English or other appropriate languages
- **📋 Summarize Text**: Create concise summaries of longer text

### Smart Text Selection
- **Auto-Select All**: If no text is selected, automatically processes the entire document
- **Selection-Aware**: Works with any selected text portion
- **Unicode Support**: Full support for international characters and languages

### User-Friendly Interface
- **Settings Dialog**: Easy configuration of OpenAI API key with connection testing
- **Input Dialog**: Custom prompt feature with multi-line text input
- **Error Handling**: Clear error messages and robust operation

## 📋 Requirements

- **Notepad++**: Version 7.0 or later: 64-bit (x64), 32-bit (x86) or ARM64
- **OpenAI API Key**: Get one from [OpenAI Platform](https://platform.openai.com/api-keys)
- **Operating System**: Windows 10 or later
- **Architecture**: each release has one zip per Notepad++ architecture (`_x64`, `_x86`, `_arm64`). Use the one that matches your Notepad++. Plugins Admin picks it for you.

## 📦 Installation

### Via Plugins Admin (recommended)
Once the plugin is accepted into the official
[nppPluginList](https://github.com/notepad-plus-plus/nppPluginList), you can install
it directly from Notepad++:
1. Open **Plugins → Plugins Admin…**
2. On the **Available** tab, search for **LLM Assistant**
3. Check the box and click **Install** — Notepad++ handles the download and future updates automatically

Maintainers: see [`packaging/PLUGINS_ADMIN_SUBMISSION.md`](packaging/PLUGINS_ADMIN_SUBMISSION.md)
for how releases and the Plugins Admin listing are produced.

### Quick Installation (manual)
1. **Download** the zip for your Notepad++ from the [latest release](https://github.com/Gvo87/NotepadPlusPlus-LLM-Plugin/releases/latest) (`_x64`, `_x86` or `_arm64`) and extract `LLMAssistant.dll`
2. **Create directory**: `C:\Program Files\Notepad++\plugins\LLMAssistant\` (32-bit Notepad++ on 64-bit Windows: `C:\Program Files (x86)\Notepad++\plugins\LLMAssistant\`)
3. **Copy file**: Place `LLMAssistant.dll` in the created directory
4. **Restart** Notepad++
5. **Configure**: Go to `Plugins > LLM Assistant > Settings` and enter your OpenAI API key

### Verification
After installation, you should see "LLM Assistant" in the Plugins menu with 8 options:
1. Custom Prompt
2. Enhance Text
3. Generate Code
4. Complete Text
5. Check Grammar
6. Translate Text
7. Summarize Text
8. Settings

## 🎯 Usage Guide

### Basic Usage
1. **Open** a document in Notepad++
2. **Select** text you want to process (optional - if nothing is selected, the entire document will be processed)
3. **Choose** a function from `Plugins > LLM Assistant`
4. **Wait** for the AI to process your request
5. **Review** the result that replaces your original text

### Custom Prompt Feature
The **Custom Prompt** option allows maximum flexibility:
1. Select `Plugins > LLM Assistant > Custom Prompt`
2. Enter any prompt in the dialog (e.g., "Explain this code", "Make this more formal", "Fix any bugs")
3. Your prompt will be combined with the selected text and sent to the AI
4. The result replaces the original text

### Example Prompts
- "Refactor this code for better performance"
- "Convert this to Python"
- "Make this email more professional"
- "Explain what this function does"
- "Add error handling to this code"
- "Translate this to Spanish"

## 🔧 Building from Source

### Prerequisites
- **Visual Studio 2022 or later** (any edition, or Build Tools) with the "Desktop development with C++" workload
- **Windows SDK** (installed with that workload)
- **For ARM64 builds only**: the "MSVC Build Tools for ARM64/ARM64EC (Latest)" component (on Visual Studio 2022: "MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools (Latest)"), added under Individual components in the Visual Studio Installer
- **Git** (for cloning the repository)

### Build Steps
```bash
# Clone the repository
git clone <repository-url>
cd NotepadPlusPlus-LLM-Plugin

# Build the plugin
.\build.bat

# The compiled DLL will be in:
# x64\Release\LLMAssistant.dll

# Or build for 32-bit or ARM64 Notepad++:
.\build.bat x86     # Win32\Release\LLMAssistant.dll
.\build.bat arm64   # ARM64\Release\LLMAssistant.dll
```

Notepad++ only loads a plugin built for its own architecture, so use the DLL that matches your Notepad++ (x64, x86 or ARM64).

### Development Environment
The build script automatically:
- Finds the newest Visual Studio with the C++ tools (using `vswhere`)
- Sets up the build environment
- Compiles with the project's platform toolset (v145), or with the newest installed toolset if v145 is missing (for example v143 on Visual Studio 2022)
- Links required Windows libraries, and links the C/C++ runtime statically, so the DLL doesn't need the Visual C++ Redistributable installed

`deploy.bat` builds the x64 DLL the same way, then copies it into `C:\Program Files\Notepad++\plugins\LLMAssistant\`. Run it as Administrator. It closes Notepad++ the same way its close button does, so unsaved files are handled as usual: Notepad++ asks whether to save them, or, if session snapshot is on, keeps them in its backup for the next start. It won't copy anything while Notepad++ is still open.

### Automated Tests
The portable request, JSON, and UTF-8 regression tests use Catch2. Configure them with CMake:
```bash
cmake -S . -B CMakeBuild
cmake --build CMakeBuild
ctest --test-dir CMakeBuild --output-on-failure
```
The first configure downloads the pinned Catch2 release. The tests do not require Notepad++ or an OpenAI API key.

## 🏗️ Technical Architecture

### Core Components
- **Language**: C++ using official Notepad++ plugin template
- **API Integration**: OpenAI GPT models via REST API
- **HTTP Client**: WinINet for native Windows compatibility
- **JSON Processing**: Custom lightweight JSON parser/builder (no external dependencies)
- **Text Encoding**: Full UTF-8 support for international text
- **Memory Management**: RAII principles with robust error handling
- **Threading**: Thread-safe singleton pattern for LLM service

### Plugin Architecture
```
LLMAssistant.dll
├── PluginDefinition.cpp    # Main plugin logic and menu handlers
├── LLMService.cpp          # OpenAI API integration
├── SettingsDialog.cpp      # Configuration UI
├── SimpleJson.cpp          # JSON parsing/building
├── NppPlugin.cpp           # Notepad++ plugin interface
└── Resources               # Dialog templates and constants
```

### Security Features
- **Secure Storage**: API keys stored in Windows registry with user-specific access
- **HTTPS Only**: All API communications use TLS encryption
- **No Data Persistence**: No text data stored locally by the plugin
- **Memory Safety**: Proper buffer management and cleanup
- **Error Isolation**: Exceptions contained within plugin boundaries

## ⚙️ Configuration

### API Key Setup
1. Get an OpenAI API key from [OpenAI Platform](https://platform.openai.com/api-keys)
2. Open Notepad++
3. Go to `Plugins > LLM Assistant > Settings`
4. Enter your API key
5. Click "Test Connection" to verify
6. Click "OK" to save

### Registry Storage
API keys are stored in: `HKEY_CURRENT_USER\Software\LLMAssistant\APIKey`

## 🐛 Troubleshooting

### Common Issues

**Plugin doesn't appear in menu**
- Ensure DLL is in correct directory: `C:\Program Files\Notepad++\plugins\LLMAssistant\`
- Restart Notepad++ completely
- Check that the DLL matches your Notepad++ architecture (x64, x86 or ARM64). Notepad++ can't load a DLL built for another one. **?** > **About Notepad++** shows which one you have

**"API key not configured" error**
- Go to Settings and enter your OpenAI API key
- Use "Test Connection" to verify the key works

**"No text available" error**
- Open a document with text content
- The plugin needs text to process

**Connection errors**
- Check your internet connection
- Verify your OpenAI API key is valid and has credits
- Ensure firewall allows Notepad++ to make HTTP requests

### Debug Information
If you encounter issues, check:
1. Notepad++ version (should be 7.0+)
2. Notepad++ architecture (x64, x86 or ARM64) matches the plugin zip you installed
3. API key validity on OpenAI platform
4. Available API credits/quota

## 📄 License

This project is open source. See the source code for implementation details and licensing information.

## 🤝 Contributing

Contributions are welcome! Areas for improvement:
- Additional LLM providers (Anthropic, Google, etc.)
- More predefined prompts
- Batch processing capabilities
- Plugin settings persistence
- Internationalization

## 📞 Support

For issues, questions, or feature requests:
1. Check this README for common solutions
2. Review the source code for technical details
3. Create an issue in the project repository
4. Include your Notepad++ version, Windows version, and error details

---

**Made with ❤️ for the Notepad++ community**
