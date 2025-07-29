#include "expr/cexpression.h"

#include <optional>
#include <memory>

namespace sp::expr {

typedef std::unique_ptr<CExpression> Node;

enum class TokenType {
    Invalid,
    Integer,
    Identifier,
    Multiply,
    Divide,
    Modulo,
    Add,
    Sub,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Equals,
    NotEquals,
    BoolAnd,
    BoolOr,
    Question,
    Colon,
    OpenParen,
    CloseParen,
    End,
};

class ParseContext {
public:
    const string& input;
    unsigned int current_idx = 0;              // Index of next character to process

    TokenType token_type = TokenType::Invalid; // Type of current token
    unsigned int start_idx = 0;                // Start index of current token

    const IdentifierContext& ident_ctx;

    ParseContext(const string& input, const IdentifierContext& ident_ctx) : input(input), ident_ctx(ident_ctx) {}

    bool advance() { // Advance to the next token; return true if there is a valid token (including an EOF token), false if the input was invalid
        while (peek() == ' ')
            current_idx++;

        start_idx = current_idx;
        switch (peek()) {
            case 0:
                token_type = TokenType::End;
                return true;

            #define TOK_SINGLE(CHAR, TYPE) \
            case CHAR: \
                current_idx++; \
                token_type = TokenType::TYPE; \
                return true;

            TOK_SINGLE('?', Question);
            TOK_SINGLE(':', Colon);
            TOK_SINGLE('%', Modulo);
            TOK_SINGLE('*', Multiply);
            TOK_SINGLE('/', Divide);
            TOK_SINGLE('(', OpenParen);
            TOK_SINGLE(')', CloseParen);

            #undef TOK_SINGLE

            #define TOK_DOUBLE(CHAR1, CHAR2, BOTH_TYPE) \
            case CHAR1: \
                current_idx++; \
                if (peek() == CHAR2) { \
                    current_idx++; \
                    token_type = TokenType::BOTH_TYPE; \
                    return true; \
                } \
                start_idx++; \
                current_idx++; \
                token_type = TokenType::Invalid; \
                set_error(string("after '") + CHAR1 + "': expected '" + CHAR2 + "' but got '" + token_text() + "'"); \
                return false;

            TOK_DOUBLE('=', '=', Equals);
            TOK_DOUBLE('!', '=', NotEquals);
            TOK_DOUBLE('&', '&', BoolAnd);
            TOK_DOUBLE('|', '|', BoolOr);

            #undef TOK_DOUBLE

            #define TOK_HYBRID(CHAR1, CHAR2, BOTH_TYPE, ONLY_CHAR1_TYPE) \
            case CHAR1: \
                current_idx++; \
                if (peek() == CHAR2) { \
                    current_idx++; \
                    token_type = TokenType::BOTH_TYPE; \
                    return true; \
                } \
                token_type = TokenType::ONLY_CHAR1_TYPE; \
                return true;

            TOK_HYBRID('>', '=', GreaterEqual, Greater);
            TOK_HYBRID('<', '=', LessEqual,    Less);

            #undef TOK_HYBRID

            default:
                if (isdigit(peek())) {
                    while (isdigit(peek())) {
                        current_idx++;
                    }
                    token_type = TokenType::Integer;
                    return true;
                }
                if (isalpha(peek()) || peek() == '_') {
                    while (isalnum(peek()) || peek() == '_') {
                        current_idx++;
                    }
                    token_type = TokenType::Identifier;
                    return true;
                }

                current_idx++;
                set_error("unexpected character '" + token_text() + "'");
                token_type = TokenType::Invalid;
                return false;
        }
    }

    char peek() { // Get the next character to process
        if (current_idx >= input.length())
            return 0;

        return input[current_idx];
    }

    const string token_text() { // Get the text of the current token
        return input.substr(start_idx, current_idx);
    }
    const string remaining_text() { // Get the remaining text in the input, in quotes, or "<eof>" if there is no more input
        if (input.length() <= start_idx)
            return "<eof>";
        return "'" + input.substr(start_idx) + "'";
    }

    bool token_is_any(std::initializer_list<TokenType> types) { // Check if the current token is any of `types`
        for (auto ty : types)
            if (token_type == ty)
                return true;

        return false;
    }

    string error = "";
    bool expect(TokenType ty, string expect) { // Check if the current token is `ty`; if it is, advance and return true; otherwise, set an error and return false
        if (token_type == ty) {
            if (!advance()) return false;
            return true;
        }

        set_error("expected '" + expect + "', got " + remaining_text());
        return false;
    }

    void set_error(string message) { // Set an error at the current position in the input
        error = string("at {index}: {message}").format({
            {"index", start_idx},
            {"message", message},
        });
    }
};

// Parse functions and AST nodes

// forward declare so parens can loop back around the precedence order
std::optional<Node> parse_parens(ParseContext& ctx);

// n 1234
class IdentifierNode : public CExpression {
    string ident;
public:
    IdentifierNode(string ident) : ident(ident) {}
    int evaluate(const IdentifierContext& context) override {
        return context.get_identifier_value(ident).value_or(0);
    }
};

class IntegerNode : public CExpression {
    int value;
public:
    IntegerNode(int value) : value(value) {}
    int evaluate(const IdentifierContext& context) override { return value; }
};

std::optional<Node> parse_value(ParseContext& ctx) {
    switch (ctx.token_type) {
        case TokenType::Identifier:
            {
                auto value = ctx.token_text();
                if (!ctx.ident_ctx.identifier_exists(value)) {
                    ctx.set_error("name '" + value + "' does not exist");
                    return {};
                }
                if (!ctx.advance()) return {};
                return Node(new IdentifierNode(value));
            }
        case TokenType::Integer:
            {
                auto value = ctx.token_text().toInt();
                if (!ctx.advance()) return {};
                return Node(new IntegerNode(value));
            }
        case TokenType::OpenParen:
            return parse_parens(ctx);
        default:
            ctx.set_error("expected 'n', an integer, or an open parenthesis, got " + ctx.remaining_text());
            return {};
    }
}

// * / % + - > >= < <= == != && ||
class BinaryNode : public CExpression {
    TokenType operation;
    Node lhs, rhs;
public:
    BinaryNode(TokenType operation, Node lhs, Node rhs) : operation(operation), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    int evaluate(const IdentifierContext& ctx) override {
        auto left = lhs->evaluate(ctx);

        // boolean op short-circuiting
        if (operation == TokenType::BoolAnd && !left)
            return 0;
        if (operation == TokenType::BoolOr && left)
            return 1;

        auto right = rhs->evaluate(ctx);

        switch (operation) {
            case TokenType::Multiply:  return left * right;
            case TokenType::Divide:    return (right == 0) ? left : left / right;
            case TokenType::Modulo:    return (right == 0) ? left : left % right;

            case TokenType::Add:       return left + right;
            case TokenType::Sub:       return left - right;

            case TokenType::Greater:      return left >  right;
            case TokenType::GreaterEqual: return left >= right;
            case TokenType::Less:         return left <  right;
            case TokenType::LessEqual:    return left <= right;

            case TokenType::Equals:    return left == right;
            case TokenType::NotEquals: return left != right;

            case TokenType::BoolAnd:   return left && right;

            case TokenType::BoolOr:    return left || right;

            default:
                // TODO error
                return 0;
        }
    }
};

#define PARSE_BINARY(FUNC, SUBFUNC, ...) \
    std::optional<Node> FUNC(ParseContext& ctx) { \
        auto lhs = SUBFUNC(ctx); \
        if (!lhs) return {}; \
        \
        while (ctx.token_is_any({__VA_ARGS__})) { \
            auto type = ctx.token_type; \
            if (!ctx.advance()) return {}; \
            auto rhs = SUBFUNC(ctx); \
            if (!rhs) return {}; \
            \
            lhs = Node(new BinaryNode(type, std::move(*lhs), std::move(*rhs))); \
        } \
        \
        return lhs; \
    }

PARSE_BINARY(parse_muldiv,     parse_value,      TokenType::Multiply, TokenType::Divide, TokenType::Modulo)
PARSE_BINARY(parse_addsub,     parse_muldiv,     TokenType::Add, TokenType::Sub);
PARSE_BINARY(parse_inequality, parse_addsub,     TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual)
PARSE_BINARY(parse_equality,   parse_inequality, TokenType::Equals, TokenType::NotEquals)
PARSE_BINARY(parse_bool_and,   parse_equality,   TokenType::BoolAnd);
PARSE_BINARY(parse_bool_or,    parse_bool_and,   TokenType::BoolOr);

// ?:
class TernaryNode : public CExpression {
    Node condition, if_true, if_false;
public:
    TernaryNode(Node condition, Node if_true, Node if_false) : condition(std::move(condition)), if_true(std::move(if_true)), if_false(std::move(if_false)) {}
    int evaluate(const IdentifierContext& ctx) override { return condition->evaluate(ctx) ? if_true->evaluate(ctx) : if_false->evaluate(ctx); }
};

std::optional<Node> parse_ternary(ParseContext& ctx) {
    auto lhs = parse_bool_or(ctx);
    if (!lhs) return {};

    if (ctx.token_type != TokenType::Question)
        return lhs;
    if (!ctx.advance()) return {};

    auto mid = parse_bool_or(ctx);
    if (!mid) return {};

    if (!ctx.expect(TokenType::Colon, ":")) return {};

    auto rhs = parse_ternary(ctx);
    if (!rhs) return {};

    return Node(new TernaryNode(std::move(*lhs), std::move(*mid), std::move(*rhs)));
}

// ()
std::optional<Node> parse_parens(ParseContext& ctx) {
    if (ctx.token_type != TokenType::OpenParen)
        return parse_ternary(ctx);
    if (!ctx.advance()) return {};

    auto inner = parse_ternary(ctx);
    if (!inner) return {};

    if (!ctx.expect(TokenType::CloseParen, ")")) return {};

    return inner;
}

// Parse entrypoint
std::unique_ptr<CExpression> CExpression::parse(const string& input, const IdentifierContext& ident_ctx, string& error) {
    auto ctx = ParseContext(input, ident_ctx);

    if (!ctx.advance()) {
        error = ctx.error;
        return nullptr;
    }

    auto result = parse_parens(ctx);
    if (!result || ctx.error != "") {
        error = ctx.error;
        return nullptr;
    }

    if (ctx.token_type == TokenType::End)
        return std::move(*result);

    error = string("at {index}: extra input: {extra}").format({
        {"index", ctx.current_idx},
        {"extra", ctx.remaining_text()},
    });
    return nullptr;
}

}
