#pragma once

#include "common.h"
#include "formula.h"

class Sheet;

class Cell: public CellInterface {
public:
    explicit Cell(Sheet &sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    Sheet &sheet_;
    std::unique_ptr<Impl> impl_;

    std::vector<Cell*> in_nodes_;
    std::vector<Cell*> out_nodes_;

    void ThrowIfCircularDependency(std::vector<Position> &positions);
    void InvalidateCache();
    void TryInstallFormulaCell(Impl *new_impl);
    void UninstallFormulaCell();
};

