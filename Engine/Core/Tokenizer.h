#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Engine {

// Hand-rolled tokenizer shared by every simple text data format this engine
// reads (maps, materials, and whatever data-driven format comes next):
// whitespace and "//" line comments are skipped between tokens; '{', '}',
// '(', ')' are always their own token; quoted strings return their
// contents; anything else is read as a bareword/number up to the next
// delimiter.
class Tokenizer {
public:
    explicit Tokenizer(const std::string& text) : m_text(text) {}

    bool NextToken(std::string& outToken) {
        SkipWhitespaceAndComments();
        if (m_pos >= m_text.size()) return false;

        const char c = m_text[m_pos];
        if (c == '{' || c == '}' || c == '(' || c == ')') {
            outToken = std::string(1, c);
            ++m_pos;
            return true;
        }
        if (c == '"') {
            ++m_pos;
            std::string value;
            while (m_pos < m_text.size() && m_text[m_pos] != '"') {
                value += m_text[m_pos++];
            }
            if (m_pos < m_text.size()) ++m_pos; // consume closing quote
            outToken = value;
            return true;
        }

        std::string value;
        while (m_pos < m_text.size() && !IsDelimiter(m_text[m_pos])) {
            value += m_text[m_pos++];
        }
        outToken = value;
        return true;
    }

    // Non-consuming lookahead, used to decide whether a block has more
    // entries or has reached its closing brace.
    bool PeekToken(std::string& outToken) {
        const size_t saved = m_pos;
        const bool ok = NextToken(outToken);
        m_pos = saved;
        return ok;
    }

private:
    static bool IsDelimiter(char c) {
        return std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}' || c == '(' || c == ')' || c == '"';
    }

    void SkipWhitespaceAndComments() {
        while (m_pos < m_text.size()) {
            if (std::isspace(static_cast<unsigned char>(m_text[m_pos]))) {
                ++m_pos;
            } else if (m_text[m_pos] == '/' && m_pos + 1 < m_text.size() && m_text[m_pos + 1] == '/') {
                while (m_pos < m_text.size() && m_text[m_pos] != '\n') ++m_pos;
            } else {
                break;
            }
        }
    }

    const std::string& m_text;
    size_t m_pos = 0;
};

// Consumes one token and fails (logging to stderr with `context` naming the
// caller, e.g. "MapLoader") if it doesn't exactly match `expected`.
bool ExpectToken(Tokenizer& tok, const std::string& expected, const char* context);

// Parses "( x y z )" into a vec3.
bool ParseVec3(Tokenizer& tok, glm::vec3& out, const char* context);

} // namespace Engine
