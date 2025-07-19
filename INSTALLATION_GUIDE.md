# LLM Assistant Plugin - Installation Guide

## Quick Start

### Step 1: Download
- Get the plugin file: `LLMAssistant.dll` from the `x64\Release\` directory

### Step 2: Install
1. **Create the plugin directory:**
   ```
   C:\Program Files\Notepad++\plugins\LLMAssistant\
   ```

2. **Copy the plugin file:**
   - Place `LLMAssistant.dll` in the directory you just created

3. **Restart Notepad++**
   - Close Notepad++ completely
   - Reopen Notepad++

### Step 3: Configure
1. **Open Settings:**
   - Go to `Plugins > LLM Assistant > Settings`

2. **Enter API Key:**
   - Get your OpenAI API key from: https://platform.openai.com/api-keys
   - Paste it in the API Key field
   - Click "Test Connection" to verify it works
   - Click "OK" to save

### Step 4: Test
1. **Open a document** with some text
2. **Select text** (or leave nothing selected to process entire document)
3. **Try a feature:** Go to `Plugins > LLM Assistant > Custom Prompt`
4. **Enter a prompt** like "Summarize this text"
5. **See the magic happen!**

## Menu Options

After installation, you'll see these 8 options in `Plugins > LLM Assistant`:

1. **Custom Prompt** - Enter any prompt you want
2. **Enhance Text** - Improve and refine text
3. **Generate Code** - Create code from descriptions
4. **Complete Text** - Continue partial text
5. **Check Grammar** - Fix grammar issues
6. **Translate Text** - Translate to other languages
7. **Summarize Text** - Create summaries
8. **Settings** - Configure your API key

## Troubleshooting

### Plugin doesn't appear
- Check the directory path: `C:\Program Files\Notepad++\plugins\LLMAssistant\`
- Make sure you have 64-bit Notepad++
- Restart Notepad++ completely

### "API key not configured" error
- Go to Settings and enter your OpenAI API key
- Make sure the key is valid and has credits

### "No text available" error
- Open a document with text content
- The plugin needs text to process

## System Requirements

- **Notepad++**: Version 7.0+ (64-bit recommended)
- **Windows**: 10 or later (64-bit)
- **Internet**: Required for API calls
- **OpenAI API Key**: Required for LLM functionality

## Support

If you encounter issues:
1. Check this guide first
2. Verify your API key works on OpenAI's website
3. Make sure you have the correct directory structure
4. Try restarting Notepad++ after installation

---

**Enjoy your AI-powered text editing experience!**
