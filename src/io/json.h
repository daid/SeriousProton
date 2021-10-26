#ifndef SP_JSON_H
#define SP_JSON_H

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace sp {
    namespace json {
        // nlohmann uses exceptions by default - these are helper to provide exception-less parsing with diagnostics.
        template<typename InputType>
        std::optional<nlohmann::json> parse(InputType&& i, std::string& error, const bool ignore_comments = false);

        template<typename IteratorType>
        std::optional<nlohmann::json> parse(IteratorType first, IteratorType last, std::string& error, const bool ignore_comments = false);
        
        namespace details {
            class JsonParser : public nlohmann::detail::json_sax_dom_parser<nlohmann::json>
            {
            public:
                JsonParser(nlohmann::json& j, std::string& error)
                : nlohmann::detail::json_sax_dom_parser<nlohmann::json>(j, false), error{error}
                {}

                bool parse_error(std::size_t /*position*/,
                                const std::string& /*last_token*/,
                                const nlohmann::json::exception& ex)
                {
                    error = ex.what();
                    return false;
                }
            private:
                std::string& error;
            };
        } // namespace details
    } // namespace json
    
} // namespace sp


template<typename InputType>
std::optional<nlohmann::json> sp::json::parse(InputType&& i, std::string& error, const bool ignore_comments)
{
    nlohmann::json result;
    details::JsonParser parser(result, error);
    constexpr auto strict_mode = true;
    constexpr auto input_format = nlohmann::json::input_format_t::json;
    auto success = nlohmann::json::sax_parse(std::forward<InputType>(i), &parser, input_format, strict_mode, ignore_comments);
    
    return success ? std::make_optional(result) : std::optional<nlohmann::json>{};
}

template<typename IteratorType>
std::optional<nlohmann::json> sp::json::parse(IteratorType first, IteratorType last, std::string& error, const bool ignore_comments)
{
    nlohmann::json result;
    details::JsonParser parser(result, error);

    constexpr auto strict_mode = true;
    constexpr auto input_format = nlohmann::json::input_format_t::json;
    
    auto success = nlohmann::json::sax_parse(first, last, &parser, input_format, strict_mode, ignore_comments);
    
    return success ? std::make_optional(result) : std::optional<nlohmann::json>{};
}
#endif // SP_JSON_H