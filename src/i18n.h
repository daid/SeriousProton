#ifndef I18N_H
#define I18N_H

#include "stringImproved.h"
#include <unordered_map>
#include <memory>
#include "expr/cexpression.h"

//Translate a string with a loaded translation.
// If no translation was loaded, return the origonal string unmodified.
// There functions are not in the i18n namespace to prevent very long identifiers.
const string& tr(const string& input);
const string& tr(const string& context, const string& input);
const string& trn(int n, const string& singular, const string& plural);
const string& trn(int n, const string& context, const string& singular, const string& plural);

//Mark a string for translation without actually translating it.
// This can be used in case the actual translation needs to happen in a different part of the code
// compared to the string location.
static inline const string& trMark(const string& input) { return input; }
static inline const string& trMark(const char* context, const string& input) { return input; }

namespace i18n {

//Load a translation file.
//  This can be a gettext .po or .mo file.
//  Multiple files can be loaded and the translations will be merged.
//  Returns true if loading was succesful.
bool load(const string& resource_name);
void reset();

class Catalogue
{
public:
    static bool load(const string& resource_name);
    static void reset();
    static const Catalogue* get();
    static std::unique_ptr<Catalogue> create(const string& resource_name);

    const string& tr(const string& input) const;
    const string& tr(const string& context, const string& input) const;
    const string& trn(int n, const string& singular, const string& plural) const;
    const string& trn(int n, const string& context, const string& singular, const string& plural) const;

    ~Catalogue();

private:
    Catalogue();

    bool load_resource(const string& resource);
    void process_headers(const string& headers);
    int nplurals = 1;
    std::unique_ptr<sp::expr::CExpression> plural_expression;
    std::unordered_map<string, std::unordered_map<string, std::vector<string>>> entries;

    static std::unique_ptr<Catalogue> instance;
};

}//!namespace i18n

#endif//I18N_H
