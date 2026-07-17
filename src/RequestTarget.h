#ifndef REQUESTTARGET_H
#define REQUESTTARGET_H

struct TextRange
{
    int start;
    int end;
    bool usesWholeDocument;
};

TextRange resolveTextRange(int selectionStart, int selectionEnd, int documentLength);
bool isTextRangeValid(const TextRange& range, int documentLength);
bool isRevisionCurrent(unsigned long capturedRevision, unsigned long currentRevision);

#endif
