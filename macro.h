
enum MacroTypes
{
    mt_textsub,  // macro as direct a->b text substitution
    mt_function, // macro as function
};

struct Macro
{
    char *inVal;
    char *outVal;

    // only valid if type == mt_function
    char **paramsList; // array of strings naming the params of this macro function
    unsigned nParams;  // size of the array of params

    enum MacroTypes type;
};

struct Macro *macro_new(char *inVal, char *outVal, enum MacroTypes type);

void macro_free(struct Macro *m);
