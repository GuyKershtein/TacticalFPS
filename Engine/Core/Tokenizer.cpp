#include "Tokenizer.h"

#include <cstdio>
#include <cstdlib>

namespace Engine {

bool ExpectToken(Tokenizer& tok, const std::string& expected, const char* context) {
    std::string t;
    if (!tok.NextToken(t) || t != expected) {
        std::fprintf(stderr, "[%s] Expected '%s' but got '%s'\n", context, expected.c_str(), t.c_str());
        return false;
    }
    return true;
}

bool ParseVec3(Tokenizer& tok, glm::vec3& out, const char* context) {
    if (!ExpectToken(tok, "(", context)) return false;
    std::string x, y, z;
    if (!tok.NextToken(x) || !tok.NextToken(y) || !tok.NextToken(z)) return false;
    if (!ExpectToken(tok, ")", context)) return false;
    out = glm::vec3(std::strtof(x.c_str(), nullptr), std::strtof(y.c_str(), nullptr), std::strtof(z.c_str(), nullptr));
    return true;
}

} // namespace Engine
