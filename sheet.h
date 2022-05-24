#pragma once

#include "cell.h"
#include "common.h"

#include <deque>

class Sheet: public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream &output) const override;
    void PrintTexts(std::ostream &output) const override;

private:
    std::deque<std::deque<std::unique_ptr<CellInterface>>> sheet_;

    enum class PrintType {
        Values,
        Texts
    };

    void Print(std::ostream &output, PrintType pt) const;
    void UpSizeIfNeed(const Position& pos);
    void DownSizeIfNeed(const Position& pos);
};
