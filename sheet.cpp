#include "cell.h"
#include "common.h"
#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() = default;

inline void ThrowInvalidPosition(Position &pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position"s);
    }
}

void Sheet::UpSizeIfNeed(const Position& pos) {
    if (sheet_.size() <= static_cast<size_t>(pos.row)) {
        sheet_.resize(pos.row + 1);
    }
    if (sheet_[pos.row].size() <= static_cast<size_t>(pos.col)) {
        sheet_[pos.row].resize(pos.col + 1);
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    ThrowInvalidPosition(pos);
    UpSizeIfNeed(pos);
    auto& cell = sheet_[pos.row][pos.col];
    if (!cell) {
        cell = std::make_unique<Cell>(*this);
    }
    dynamic_cast<Cell*>(cell.get())->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    ThrowInvalidPosition(pos);
    if (sheet_.size() > static_cast<size_t>(pos.row) && sheet_[pos.row].size() > static_cast<size_t>(pos.col)) {
        return sheet_[pos.row][pos.col].get();
    }
    return nullptr;
}

void Sheet::DownSizeIfNeed(const Position& pos) {
    if (sheet_[pos.row].size() - 1 == static_cast<size_t>(pos.col)) {
        int col_len;
        for (col_len = sheet_[pos.row].size() - 1; col_len >= 0; --col_len) {
            if (sheet_[pos.row][col_len] != nullptr) {
                sheet_[pos.row].resize(col_len + 1);
                break;
            }
        }
        if (col_len == -1) {
            sheet_[pos.row].clear();
        }
        if (sheet_[pos.row].size() == 0) {
            int row_len;
            for (row_len = sheet_.size() - 1; row_len >= 0; --row_len) {
                if (sheet_[row_len].size() != 0) {
                    sheet_.resize(row_len + 1);
                    break;
                }
            }
            if (row_len == -1) {
                sheet_.clear();
            }
        }
    }
}

void Sheet::ClearCell(Position pos) {
    ThrowInvalidPosition(pos);
    if (sheet_.size() > static_cast<size_t>(pos.row) && sheet_[pos.row].size() > static_cast<size_t>(pos.col)) {

        // далее reset вызывает деуструктор Cell::~Cell(), в котором вызывается метод UninstallFormulaCell(),
        // очищающий связи ячейки и инвалидирующий кэш, после чего объект удаляется и указатель становится nullptr
        sheet_[pos.row][pos.col].reset(nullptr);

        // Выполняется "обрезка" таблицы, если очищенная ячейка была крайней справа в своей строке, то
        // то орезается свой массив столбцов. Также если ячейка была единственной в самой нижней строке, тогда обрезается еще и массив строк.
        DownSizeIfNeed(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    Size size;
    for (auto &row : sheet_) {
        if (row.size() > static_cast<size_t>(size.cols)) {
            size.cols = static_cast<size_t>(row.size());
        }
    }
    size.rows = sheet_.size();
    return size;
}

inline std::ostream& operator<<(std::ostream &output, const CellInterface::Value &value) {
    std::visit([&](const auto &x) {
        output << x;
    },
    value);
    return output;
}

void Sheet::Print(std::ostream &output, PrintType pt) const {
    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (sheet_[row].size() > static_cast<size_t>(col)) {
                if (sheet_[row][col]) {
                    (pt == PrintType::Values) ?
                            output << sheet_[row][col]->GetValue() : output << sheet_[row][col]->GetText();
                }
            }
            if (col == size.cols - 1) {
                output << '\n';
            } else {
                output << '\t';
            }
        }
    }
}

void Sheet::PrintValues(std::ostream &output) const {
    Print(output, PrintType::Values);
}

void Sheet::PrintTexts(std::ostream &output) const {
    Print(output, PrintType::Texts);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
