#ifndef MODELPARAMS_H
#define MODELPARAMS_H

#include <string>

// Reasoning models (every GPT model from GPT-5 onward and the o-series) reject
// the classic "max_tokens" and any non-default "temperature". They require
// "max_completion_tokens" and no temperature override.
bool isReasoningModel(const std::wstring& model);

// Models OpenAI has deprecated or already shut down: the GPT-3.5/GPT-4
// families, the original GPT-5 generation, GPT-5.1 and the "*-chat-latest"
// variants. Saved settings using one are switched to the default model.
bool isRetiredModel(const std::wstring& model);

#endif
