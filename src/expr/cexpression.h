#pragma once

#include "stringImproved.h"
#include <memory>
#include <optional>

namespace sp::expr {

class IdentifierContext
{
public:
    // Get the value of the identifier `ident`, or empty if there is no such identifier.
    virtual std::optional<int> get_identifier_value(const string& ident) const = 0;

    // Check whether the identifier `ident` exists.
    virtual bool identifier_exists(const string& ident) const {
        return get_identifier_value(ident).has_value();
    }
};

class CExpression
{
public:
    static std::unique_ptr<CExpression> parse(const string& input, const IdentifierContext& ctx, string& error);

    virtual ~CExpression() = default;

    virtual int evaluate(const IdentifierContext& context) = 0;
};

}
