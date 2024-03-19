#include "preprocessor-buffering.h"

void defineMacro(struct PreprocessorContext *c, char *token, char *outVal);

void undefineMacro(struct PreprocessorContext *c, char *token);

void attemptMacroSubstitution(struct PreprocessorContext *c, char stillParsing);
