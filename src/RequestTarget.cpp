#include "RequestTarget.h"

TextRange resolveTextRange(int selectionStart, int selectionEnd, int documentLength)
{
    if (documentLength <= 0) {
        return { 0, 0, false };
    }

    if (selectionStart >= 0 && selectionEnd > selectionStart && selectionEnd <= documentLength) {
        return { selectionStart, selectionEnd, false };
    }

    return { 0, documentLength, true };
}

bool isTextRangeValid(const TextRange& range, int documentLength)
{
    return documentLength > 0
        && range.start >= 0
        && range.end > range.start
        && range.end <= documentLength;
}

bool isRevisionCurrent(unsigned long capturedRevision, unsigned long currentRevision)
{
    return capturedRevision == currentRevision;
}
