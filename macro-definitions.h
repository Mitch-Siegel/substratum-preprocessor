#include "preprocessor-buffering.h"

void defineTextSubMacro(struct PreprocessorContext *c, char *token, char *outVal);

void defineFunctionMacro(struct PreprocessorContext *c, char *token, char *spaceSeparatedParamsList, char *outVal);

void undefineMacro(struct PreprocessorContext *c, char *token);

void attemptMacroSubstitutionToBuffer(struct PreprocessorContext *c, struct TextBuffer *b, char stillParsing);

void attemptMacroSubstitution(struct PreprocessorContext *c, char stillParsing);
