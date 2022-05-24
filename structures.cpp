#include "common.h"

#include <cctype>
#include <cmath>
#include <optional>
#include <sstream>
#include <tuple>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;
const int ALPHA_OFFSET = 65;

const Position Position::NONE = { -1, -1 };

bool Position::operator==(const Position rhs) const {
    return std::tie(row, col) == std::tie(rhs.row, rhs.col);
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    if (*this == NONE) {
        return false;
    }
    if (row < 0 || col < 0) {
        return false;
    }
    if (row + 1 > MAX_ROWS || col + 1 > MAX_COLS) {
        return false;
    }
    return true;
}

inline int LetToInt(char letter) {
    return static_cast<int>(letter) - ALPHA_OFFSET;
}

inline char IntToChar(int character) {
    return static_cast<char>(character + ALPHA_OFFSET);
}

std::string IntToColStr(int col) {
    std::string ret;
    int i = (col - 1) / LETTERS;
    int j = (col - 1) % LETTERS;
    while (i > 0) {
        ret = IntToChar(j) + ret;
        j = (i - 1) % LETTERS;
        i = (i - 1) / LETTERS;
    }
    return IntToChar(j) + ret;
}

std::string Position::ToString() const {
    if (this->IsValid()) {
        return IntToColStr(col + 1) + std::to_string(row + 1);
    }
    return {};
}

std::optional<size_t> Split(std::string_view str) {
    size_t alpha_len { };
    while (alpha_len < str.length() && std::isupper(static_cast<unsigned char>(str[alpha_len]))) {
        ++alpha_len;
    }
    if (alpha_len == 0 || alpha_len > MAX_POS_LETTER_COUNT) {
        return std::nullopt;
    }
    size_t number_len { };
    while (alpha_len + number_len < str.length()) {
        if (!std::isdigit(static_cast<unsigned char>(str[alpha_len + number_len]))) {
            return std::nullopt;
        }
        ++number_len;
    }
    if (number_len == 0 || alpha_len + number_len > MAX_POSITION_LENGTH) {
        return std::nullopt;
    }
    return alpha_len;
}

int ColStrToInt(std::string_view str) {
    size_t len = str.length();
    if (len == 1) {
        return LetToInt(str[0]);
    }
    int sym = LetToInt(str[0]) + 1;
    int term = sym * std::pow(LETTERS, len - 1);
    return term + ColStrToInt(str.substr(1));
}

Position Position::FromString(std::string_view str) {
    if (auto col_len = Split(str)) {
        Position ret { std::stoi(std::string(str.substr(*col_len))) - 1, ColStrToInt(str.substr(0, *col_len)) };
        if (ret.IsValid()) {
            return ret;
        }
    }
    return NONE;
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}
