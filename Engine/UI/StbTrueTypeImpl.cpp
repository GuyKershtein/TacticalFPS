// The single translation unit that instantiates stb_truetype's
// implementation, exactly the pattern MiniaudioImpl.cpp already established
// for miniaudio: every other file only sees the declarations.
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
