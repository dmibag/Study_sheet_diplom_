#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream &output, FormulaError fe) {
    return output << '#' << fe.ToString() << '!';
}

namespace {
class Formula: public FormulaInterface {
private:
    struct VarVal {
        double operator()(double var) const {
            return var;
        }
        double operator()(FormulaError var) const {
            throw var;
        }
        double operator()(std::string var) const {
            if (var.length() == 0) {
                return 0.0;
            }
            std::istringstream str(var);
            double arg;
            str >> arg;
            if (str.eof()) {
                return arg;
            } else {
                throw FormulaError(FormulaError::Category::Value);
            }
        }
    };

public:
    explicit Formula(std::string expression) :
            ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface &sheet) const override {
        std::function<double(const Position*)> args = [&sheet](const Position *pos) {
            auto cell = sheet.GetCell(*pos);
            return (cell) ? std::visit(VarVal { }, cell->GetValue()) : 0.0;
        };

        try {
            return ast_.Execute(args);
        } catch (FormulaError &fe) {
            return fe;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream str;
        ast_.PrintFormula(str);
        return str.str();
    }

    std::vector<Position> GetReferencedCells() const {
        std::vector<Position> vec(ast_.GetCells().begin(), ast_.GetCells().end());
        vec.erase(unique(vec.begin(), vec.end()), vec.end());
        return vec;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
