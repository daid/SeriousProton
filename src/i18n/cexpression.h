#pragma once

#include "stringImproved.h"
#include <memory>

namespace i18n {

class CExpression
{
public:
    static std::unique_ptr<CExpression> parse(const string& input, string& error);

    virtual ~CExpression() = default;

    virtual int evaluate(int n) = 0;
};

}
