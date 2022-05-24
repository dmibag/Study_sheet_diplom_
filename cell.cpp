#include "cell.h"
#include "sheet.h"

#include <algorithm>
#include <forward_list>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl: public Cell::Impl {
public:
    EmptyImpl() = default;

    CellInterface::Value GetValue() const override {
        return "";
    }

    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl: public Cell::Impl {
public:
    explicit TextImpl(std::string str) :
            str_(std::move(str)) {
        if (str_.length() == 0) {
            throw CellImplException("Trying to create empty text cell as TextImpl");
        }
    }

    CellInterface::Value GetValue() const override {
        if (str_.length() > 0 && str_[0] == ESCAPE_SIGN) {
            return str_.substr(1);
        }
        return str_;
    }

    std::string GetText() const override {
        return str_;
    }
private:
    std::string str_;
};

class Cell::FormulaImpl: public Cell::Impl {
public:
    explicit FormulaImpl(std::string str, const SheetInterface &sheet) :
            sheet_(sheet) {
        if (str.length() > 1 && str[0] == FORMULA_SIGN) {
            throw CellImplException("Trying to create FormulaImpl with str=" + str);
        }
        // В случае, если str не является корректной формулой, будет выброшено исключение FormulaError
        // в методе ParseFormulaAST
        formula_ = ParseFormula(str);
    }

    CellInterface::Value GetValue() const {
        return cache_.value_or(std::visit([&](const auto &arg) -> CellInterface::Value {
            cache_ = arg;
            return arg;
        }, formula_->Evaluate(sheet_)));
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }

    void ClearCache() {
        cache_.reset();
    }

    bool HasCache() const {
        return cache_.has_value();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface &sheet_;
    mutable std::optional<CellInterface::Value> cache_;
};

Cell::Cell(Sheet &sheet) :
        sheet_(sheet), impl_(std::make_unique<Cell::EmptyImpl>()) {
}

Cell::~Cell() {
    UninstallFormulaCell();
}

void Cell::ThrowIfCircularDependency(std::vector<Position> &positions) {
    std::forward_list<Cell*> next_cells;
    for (Position &pos : positions) {
        Cell *cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));
        next_cells.push_front(cell);
    }
    std::unordered_set<Cell*> visited;
    while (!next_cells.empty()) {
        auto [it, good] = visited.insert(next_cells.front());
        next_cells.pop_front();
        if (good) {
            if (this == *it) {
                throw CircularDependencyException("Circular references");
            }
            if (*it) {
                next_cells.insert_after(next_cells.before_begin(), (*it)->out_nodes_.begin(), (*it)->out_nodes_.end());
            }
        }
    }
}

void Cell::InvalidateCache() {
    std::forward_list<Cell*> next_cells;
    for (Cell *cell : in_nodes_) {
        next_cells.push_front(cell);
    }
    while (!next_cells.empty()) {
        Cell *inspect_cell = next_cells.front();
        next_cells.pop_front();
        FormulaImpl *formula_impl = dynamic_cast<FormulaImpl*>(inspect_cell->impl_.get());
        if (formula_impl) {
            if (formula_impl->HasCache()) {
                formula_impl->ClearCache();
                for (Cell *next_cell : inspect_cell->in_nodes_) {
                    next_cells.push_front(next_cell);
                }
            }
        }
    }
}

void Cell::TryInstallFormulaCell(Impl *new_impl) {
    FormulaImpl *formula_impl = dynamic_cast<FormulaImpl*>(new_impl);
    if (formula_impl) {
        auto out_cells = formula_impl->GetReferencedCells();

        // Проверяем циклическую зависимость для временного объекта
        ThrowIfCircularDependency(out_cells);

        // Очищаем зависимости текущей ячейки и инвалидируем кэш
        UninstallFormulaCell();

        for (Position &pos : out_cells) {
            // Создаем пустые ячейки, на которые ссылается новая формула, в случае, если их не существует
            if (sheet_.GetCell(pos) == nullptr) {
                sheet_.SetCell(pos, { });
            }
            Cell *cell = dynamic_cast<Cell*>(sheet_.GetCell(pos));

            // Прописываем себя в ячеках, на которые ссылается новая формула
            cell->in_nodes_.push_back(this);

            // Обновляем свои исходящие связи
            out_nodes_.push_back(cell);
        }
    } else {
        UninstallFormulaCell();
    }

}

void Cell::UninstallFormulaCell() {
    FormulaImpl *formula_impl = dynamic_cast<FormulaImpl*>(impl_.get());
    if (formula_impl) {
        // Вычищаем входящие ноды, на которые ссылалась очищаемая ячейка
        for (auto *cell : out_nodes_) {
            auto it = std::find(cell->in_nodes_.begin(), cell->in_nodes_.end(), this);
            if (it != cell->in_nodes_.end()) {
                cell->in_nodes_.erase(it);
            }
        }

        // Очищаем свои исходящие ноды
        out_nodes_.clear();

        // Инвалидируем кэш в деревеве нод, которые ссылались на данную ячейку
        InvalidateCache();
    }
}

void Cell::Set(std::string text) {
    // Создаем подходящюю реализацию Impl
    std::unique_ptr<Impl> new_impl;
    if (text.length() > 1 && text[0] == FORMULA_SIGN) { // '='
        new_impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
    } else if (text.length()) {
        new_impl = std::make_unique<TextImpl>(text);
    } else {
        new_impl = std::make_unique<EmptyImpl>();
    }

    TryInstallFormulaCell(new_impl.get());
    impl_ = std::move(new_impl);
}

void Cell::Clear() {
    UninstallFormulaCell();
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    FormulaImpl *formula_impl = dynamic_cast<FormulaImpl*>(impl_.get());
    if (formula_impl) {
        return formula_impl->GetReferencedCells();
    }
    return {};
}

bool Cell::IsReferenced() const {
    return out_nodes_.size();
}
