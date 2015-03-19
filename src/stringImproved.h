#ifndef STRING_IMPROVED_H
#define STRING_IMPROVED_H

#include <stdlib.h>
#include <string>
#include <limits>
#include <vector>
#include <sstream>
#include <iomanip>

/*
    The improved string class. while this class is not always the most efficient in terms of execution speed.
    It does provide a lot of extra functions which makes it easier to work with strings.
    It implements the same API as python strings where possible. However, python strings are immutable, while C++ strings are mutable.
*/

#define _WHITESPACE " \n\r\t"
class string : public std::string
{
public:
    string() : std::string() {}
    string(const std::string& str) : std::string(str) {}
    string(const char* str) : std::string(str) {}
    string(const char* str, int length) : std::string(str, length) {}

    string(const char c) : std::string()
    {
        push_back(c);
    }

    string(const int nr) : std::string()
    {
        std::ostringstream stream;
        stream << nr;
        *this = stream.str();
    }

    string(const unsigned int nr) : std::string()
    {
        std::ostringstream stream;
        stream << nr;
        *this = stream.str();
    }

    string(const float nr, int decimals = 2) : std::string()
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(decimals);
        stream << nr;
        *this = stream.str();
    }

    static string hex(const int nr)
    {
        std::ostringstream stream;
        stream << std::hex << nr;
        return stream.str();
    }

    /*
        substr works the same as the [start:end] operator in python, allowing negative indexes to get the back of the string.
        It is also garanteed to be safe. So if you request an out of range index, you will get an empty string.
    */
    string substr(const int pos = 0, const int endpos = std::numeric_limits<int>::max()) const
    {
        int start = pos;
        int end = endpos;
        int len = length();
        if (start < 0)
            start = len + start;
        if (end < 0)
            end = len + end;
        if (start < 0)
        {
            end += start;
            start = 0;
        }
        len = std::min(end, len);
        if (end <= start)
        {
            return "";
        }
        return std::string::substr(start, end - start);
    }

    string operator*(const int count)
    {
        if (count <= 0)
            return "";
        string ret;
        for(int n=0; n<count; n++)
            ret += *this;
        return ret;
    }

    /*
        Return a copy of the string S with only its first character capitalized.
    */
    string capitalize()
    {
        return substr(0, 1).upper() + substr(1).lower();
    }

    /*
        Return S centered in a string of length width. Padding is done using the specified fill character (default is a space)
    */
    string center(const int width, const char fillchar=' ') const
    {
        if (width < int(length()))
            return *this;
        int right = width - length();
        int left = right / 2;
        right -= left;
        return string(fillchar) * left + *this + string(fillchar) * right;
    }

    /*
        Return the number of non-overlapping occurrences of substring sub in
        string S[start:end].  Optional arguments start and end are interpreted
        as in slice notation.
    */
    int count(const string sub) const
    {
        if (length() < sub.length())
            return 0;
        int count = 0;
        for(unsigned int n=0; n<=length() - sub.length(); n++)
        {
            if (substr(n, n + sub.length()) == sub)
                count++;
        }
        return count;
    }

    /*
        Return True if S ends with the specified suffix, False otherwise.
        With optional start, test S beginning at that position.
        With optional end, stop comparing S at that position.
    */
    bool endswith(const string suffix) const
    {
        if (suffix.length() == 0)
            return true;
        return substr(-suffix.length()) == suffix;
    }

    /*
        Return a copy of S where all tab characters are expanded using spaces.
        If tabsize is not given, a tab size of 8 characters is assumed.
    */
    string expandtabs(const int tabsize=8) const
    {
        string ret = "";
        int p = 0;
        int t;
        int start = 0;
        int end = find("\r");
        if (find("\n") > -1 && (end == -1 || find("\n") < end))
            end = find("\n");
        while((t = find("\t", p)) > -1)
        {
            while(end != -1 && end < t)
            {
                start = end + 1;
                end = find("\r", start);
                if (find("\n", start) > -1 && (end == -1 || find("\n", start) < end))
                    end = find("\n", start);
            }
            ret += substr(p, t) + string(" ") * (tabsize - ((t - start) % tabsize));
            p = t + 1;
        }
        ret += substr(p);
        return ret;
    }

    /*
        Return the lowest index in S where substring sub is found,
        such that sub is contained within s[start:end].  Optional
        arguments start and end are interpreted as in slice notation.
    */
    int find(const string sub, int start=0) const
    {
        if (sub.length() + start > length() || sub.length() < 1)
            return -1;
        for(unsigned int n=start; n<=length() - sub.length(); n++)
        {
            if(substr(n, n+sub.length()) == sub)
                return n;
        }
        return -1;
    }

    /*
        Return a formatted version of S
    */
    string format(...) const;

    /*
        Return True if all characters in S are alphanumeric
        and there is at least one character in S, False otherwise.
    */
    bool isalnum() const
    {
        int count = 0;
        for(unsigned int n=0; n<length(); n++)
        {
            if (!::isalnum((*this)[n]))
                return false;
            count++;
        }
        return count > 0;
    }

    /*
        Return True if all characters in S are alphabetic
        and there is at least one character in S, False otherwise.
    */
    bool isalpha() const
    {
        int count = 0;
        for(unsigned int n=0; n<length(); n++)
        {
            if (!::isalpha((*this)[n]))
                return false;
            count++;
        }
        return count > 0;
    }

    /*
        Return True if all characters in S are digits
        and there is at least one character in S, False otherwise.
    */
    bool isdigit() const
    {
        int count = 0;
        for(unsigned int n=0; n<length(); n++)
        {
            if (!::isdigit((*this)[n]))
                return false;
            count++;
        }
        return count > 0;
    }

    /*
        Return True if all cased characters in S are lowercase and there is
        at least one cased character in S, False otherwise.
    */
    bool islower() const
    {
        int count = 0;
        for(unsigned int n=0; n<length(); n++)
        {
            if ((*this)[n] == '\n')
                continue;
            if (!::islower((*this)[n]))
                return false;
            count++;
        }
        return count > 0;
    }

    /*
        Return True if all characters in S are whitespace
        and there is at least one character in S, False otherwise.
    */
    bool isspace() const
    {
        int count = 0;
        for(unsigned int n=0; n<length(); n++)
        {
            if (!::isspace((*this)[n]))
                return false;
            count++;
        }
        return count > 0;
    }


    /*
        Return True if S is a titlecased string and there is at least one
        character in S, i.e. uppercase characters may only follow uncased
        characters and lowercase characters only cased ones. Return False
        otherwise.
    */
    bool istitle() const
    {
        int count = 0;
        bool needUpper = true;
        for(unsigned int n=0; n<length(); n++)
        {
            if ((*this)[n] == '\n')
            {
                needUpper = true;
                continue;
            }
            if (::isalpha((*this)[n]))
            {
                if (::isupper((*this)[n]) != needUpper)
                    return false;
                needUpper = false;
            }else{
                needUpper = true;
            }
            count++;
        }
        return count > 0;
    }

    /*
        Return True if all cased characters in S are uppercase and there is
        at least one cased character in S, False otherwise.
    */
    bool isupper() const
    {
        int count = 0;
        for(unsigned int n=0; n<length(); n++)
        {
            if ((*this)[n] == '\n')
                continue;
            if (!::isupper((*this)[n]))
                return false;
            count++;
        }
        return count > 0;
    }


    /*
        Return a string which is the concatenation of the strings in the
        iterable.  The separator between elements is S.
    */
    string join(const std::vector<string> list) const
    {
        string ret;
        for(unsigned int n=0; n<list.size(); n++)
        {
            if (n > 0)
                ret += " ";
            ret += list[n];
        }
        return ret;
    }

    /*
        Return S left-justified in a string of length width. Padding is
        done using the specified fill character (default is a space).
    */
    string ljust(const int width, const char fillchar=' ') const
    {
        if (int(length()) >= width)
            return *this;
        return *this + string(fillchar) * (width - length());
    }

    /*
        Return a copy of the string S converted to lowercase.
    */
    string lower() const
    {
        string ret = *this;
        for(unsigned int n=0; n<length(); n++)
            ret[n] = tolower(ret[n]);
        return ret;
    }

    /*
        Return a copy of the string S with leading whitespace removed.
        If chars is given and not None, remove characters in chars instead.
    */
    string lstrip(const string chars=_WHITESPACE) const
    {
        int start=0;
        while(chars.find(substr(start, start+1)) > -1)
            start++;
        return substr(start);
    }

    /*
        Search for the separator sep in S, and return the part before it,
        the separator itand the part after it.  If the separator is not
        found, return S and two empty strings.
    */
    std::vector<string> partition(const string sep) const;

    /*
        Return a copy of string S with all occurrences of substring
        old replaced by new.  If the optional argument count is
        given, only the first count occurrences are replaced.
    */
    string replace(const string old, const string _new, const int count=-1) const;

    /*
        Return the highest index in S where substring sub is found,
        such that sub is contained within s[start:end].  Optional
        arguments start and end are interpreted as in slice notation.
    */
    int rfind(const string sub, int start=0) const
    {
        if (sub.length() + start > length())
            return -1;
        for(unsigned int n=length() - sub.length(); int(n)>=start; n--)
        {
            if(substr(n, n+sub.length()) == sub)
                return n;
        }
        return -1;
    }

    /*
        Return S right-justified in a string of length width. Padding is
        done using the specified fill character (default is a space)
    */
    string rjust(const int width, const char fillchar=' ') const
    {
        if (int(length()) >= width)
            return *this;
        return string(fillchar) * (width - length()) + *this;
    }

    /*
        Search for the separator sep in S, starting at the end of S, and return
        the part before it, the separator itand the part after it.  If the
        separator is not found, return two empty strings and S.
    */
    std::vector<string> rpartition(const string sep) const;

    /*
        Return a list of the words in the string S, using sep as the
        delimiter string, starting at the end of the string and working
        to the front.  If maxsplit is given, at most maxsplit splits are
        done. If sep is not specified or is None, any whitespace string
        is a separator.
    */
    std::vector<string> rsplit(const string sep=_WHITESPACE, const int maxsplit=-1) const;

    /*
        Return a copy of the string S with trailing whitespace removed.
        If chars is given and not None, remove characters in chars instead.
    */
    string rstrip(const string chars=_WHITESPACE) const
    {
        int end=length()-1;
        while(chars.find(substr(end, end+1)) > -1)
            end--;
        return substr(0, end+1);
    }

    /*
        Return a list of the words in the string S, using sep as the
        delimiter string.  If maxsplit is given, at most maxsplit
        splits are done. If sep is not specified or is None, any
        whitespace string is a separator and empty strings are removed
        from the result.
    */
    std::vector<string> split(const string sep="", int maxsplit=-1) const
    {
        std::vector<string> res;
        int start = 0;
        if(sep == "")
        {
            res = split(" ", maxsplit);
            for(unsigned int n=0; n<res.size(); n++)
            {
                if (res[n].length() < 1)
                {
                    res.erase(res.begin() + n);
                    n--;
                }
            }
            return res;
        }
        while(maxsplit != 0 && start < int(length()))
        {
            int offset = find(sep, start);
            if (offset < 0)
            {
                res.push_back(substr(start));
                return res;
            }
            res.push_back(substr(start, offset));
            start = offset + sep.length();
            if (maxsplit > 0)
                maxsplit--;
        }
        if (start < int(length()))
            res.push_back(substr(start));
        return res;
    }

    /*
        Return a list of the lines in S, breaking at line boundaries.
        Line breaks are not included in the resulting list unless keepends
        is given and true.
    */
    std::vector<string> splitlines(const bool keepends=false) const;

    /*
        Return True if S starts with the specified prefix, False otherwise.
        With optional start, test S beginning at that position.
        With optional end, stop comparing S at that position.
    */
    bool startswith(const string prefix) const
    {
        return substr(0, prefix.length()) == prefix;
    }

    /*
        Return a copy of the string S with leading and trailing
        whitespace removed.
        If chars is given and not None, remove characters in chars instead.
    */
    string strip(const string chars=_WHITESPACE) const
    {
        return lstrip(chars).rstrip(chars);
    }

    /*
        Return a copy of the string S with uppercase characters
        converted to lowercase and vice versa.
    */
    string swapcase() const
    {
        string ret = *this;
        for(unsigned int n=0; n<length(); n++)
            if (::isupper(ret[n]))
                ret[n] = ::tolower(ret[n]);
            else
                ret[n] = ::toupper(ret[n]);
        return ret;
    }

    /*
        Return a titlecased version of S, i.e. words start with uppercase
        characters, all remaining cased characters have lowercase.
    */
    string title() const
    {
        string ret = *this;
        bool needUpper = true;
        for(unsigned int n=0; n<length(); n++)
        {
            if (::isalpha(ret[n]))
            {
                if (needUpper)
                    ret[n] = ::toupper(ret[n]);
                else
                    ret[n] = ::tolower(ret[n]);
                needUpper = false;
            }else{
                needUpper = true;
            }
        }
        return ret;
    }

    /*
        Return a copy of the string S, where all characters occurring
        in the optional argument deletechars are removed, and the
        remaining characters have been mapped through the given
        translation table, which must be a string of length 256.
    */
    string translate(const string table, const string deletechars="") const;
    /*
        Return a copy of the string S converted to uppercase.
    */
    string upper() const
    {
        string ret = *this;
        for(unsigned int n=0; n<length(); n++)
            ret[n] = toupper(ret[n]);
        return ret;
    }

    /*
        Pad a numeric string S with zeros on the left, to fill a field
        of the specified width.  The string S is never truncated.
    */
    string zfill(const int width) const
    {
        if (int(length()) > width)
            return *this;
        if ((*this)[0] == '-' || (*this)[0] == '+')
            return substr(0, 1) + string("0") * (width - length()) + substr(1);
        return string("0") * (width - length()) + *this;
    }


    /* Convert this string to a number */
    float toFloat() { return atof(c_str()); }
    int toInt() { return atoi(c_str()); }
};
#undef _WHITESPACE

void __stringTest();

#endif//STRING_H
