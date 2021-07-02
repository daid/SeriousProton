#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include "stringImproved.h"

#define checkequal(a, b) { \
    if ((a) != (b)) { \
        std::cout << "" << a << "\n!=\n" << b << "" << std::endl; \
        std::cout << "For test:" << std::endl; \
        std::cout << #a << " == " << #b << std::endl; \
        error_count ++; \
    }}

void __stringTest()
{
    int error_count = 0;
    checkequal(string("hello"), string("hello").substr());
    checkequal(string("hel"), string("hello").substr(0, 3));
    checkequal(string("he"), string("hello").substr(0, -3));
    checkequal(string("llo"), string("hello").substr(-3));
    
    //test_capitalize
    checkequal(string(" hello "), string(" hello ").capitalize());
    checkequal(string("Hello "), string("Hello ").capitalize());
    checkequal(string("Hello "), string("hello ").capitalize());
    checkequal(string("Aaaa"), string("aaaa").capitalize());
    checkequal(string("Aaaa"), string("AaAa").capitalize());

    //test_count
    checkequal(3, string("aaa").count("a"));
    checkequal(0, string("aaa").count("b"));
    checkequal(3, string("aaa").count("a"));
    checkequal(0, string("aaa").count("b"));
    checkequal(3, string("aaa").count("a"));
    checkequal(0, string("aaa").count("b"));
    checkequal(0, string("aaa").count("b"));

    checkequal(1, string("").count(""));

    checkequal(0, string("").count("xx"));

    //test_find    
    checkequal(0, string("abcdefghiabc").find("abc"));
    checkequal(9, string("abcdefghiabc").find("abc", 1));
    checkequal(-1, string("abcdefghiabc").find("def", 4));

    checkequal(0, string("abc").find("", 0));
    checkequal(3, string("abc").find("", 3));
    checkequal(-1, string("abc").find("", 4));

    checkequal(0, string("").find(""));
    checkequal(-1, string("").find("xx"));
    
    checkequal(1, string("123").find("2"));
    checkequal(1, string("123").find('2'));
    checkequal(-1, string("123").find('2', 2));
    checkequal(-1, string("123").find('4'));
    checkequal(-1, string("123").find('\0'));

    //test_rfind
    checkequal(9,  string("abcdefghiabc").rfind("abc"));
    checkequal(12, string("abcdefghiabc").rfind(""));
    checkequal(0,  string("abcdefghiabc").rfind("abcd"));
    checkequal(-1, string("abcdefghiabc").rfind("abcz"));

    checkequal(3, string("abc").rfind("", 0));
    checkequal(3, string("abc").rfind("", 3));
    checkequal(-1, string("abc").rfind("", 4));

    checkequal(1, string("123").rfind("2"));
    checkequal(1, string("123").rfind('2'));
    checkequal(-1, string("123").rfind('4'));
    checkequal(-1, string("123").rfind('\0'));

    //test_lower
    checkequal(string("hello"), string("HeLLo").lower());
    checkequal(string("hello"), string("hello").lower());

    //test_upper
    checkequal(string("HELLO"), string("HeLLo").upper());
    checkequal(string("HELLO"), string("HELLO").upper());

    //test_expandtabs
    checkequal(string("a       bc"), string("a\tbc").expandtabs());
    checkequal(string("abc\rab      def"), string("abc\rab\tdef").expandtabs());
    checkequal(string("abc\rab      def\ng       hi"), string("abc\rab\tdef\ng\thi").expandtabs());
    checkequal(string("abc\rab      def\ng       hi"), string("abc\rab\tdef\ng\thi").expandtabs(8));
    checkequal(string("abc\rab  def\ng   hi"), string("abc\rab\tdef\ng\thi").expandtabs(4));
    checkequal(string("abc\r\nab  def\ng   hi"), string("abc\r\nab\tdef\ng\thi").expandtabs(4));
    checkequal(string("abc\rab      def\ng       hi"), string("abc\rab\tdef\ng\thi").expandtabs());
    checkequal(string("abc\rab      def\ng       hi"), string("abc\rab\tdef\ng\thi").expandtabs(8));
    checkequal(string("abc\r\nab\r\ndef\ng\r\nhi"), string("abc\r\nab\r\ndef\ng\r\nhi").expandtabs(4));
    checkequal(string("  a\n b"), string(" \ta\n\tb").expandtabs(1));
/*
    //test_split
    checkequal(["this", "is", "the", "split", "function"], "this is the split function", "split")

        # by whitespace
        checkequal(["a", "b", "c", "d"], "a b c d ", "split")
        checkequal(["a", "b c d"], "a b c d", "split", None, 1)
        checkequal(["a", "b", "c d"], "a b c d", "split", None, 2)
        checkequal(["a", "b", "c", "d"], "a b c d", "split", None, 3)
        checkequal(["a", "b", "c", "d"], "a b c d", "split", None, 4)
        checkequal(["a", "b", "c", "d"], "a b c d", "split", None,
                        sys.maxint-1)
        checkequal(["a b c d"], "a b c d", "split", None, 0)
        checkequal(["a b c d"], "  a b c d", "split", None, 0)
        checkequal(["a", "b", "c  d"], "a  b  c  d", "split", None, 2)

        checkequal([], "         ", "split")
        checkequal(["a"], "  a    ", "split")
        checkequal(["a", "b"], "  a    b   ", "split")
        checkequal(["a", "b   "], "  a    b   ", "split", None, 1)
        checkequal(["a", "b   c   "], "  a    b   c   ", "split", None, 1)
        checkequal(["a", "b", "c   "], "  a    b   c   ", "split", None, 2)
        checkequal(["a", "b"], "\n\ta \t\r b \v ", "split")
        aaa = " a "*20
        checkequal(["a"]*20, aaa, "split")
        checkequal(["a"] + [aaa[4:]], aaa, "split", None, 1)
        checkequal(["a"]*19 + ["a "], aaa, "split", None, 19)

        # by a char
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "split", "|")
        checkequal(["a|b|c|d"], "a|b|c|d", "split", "|", 0)
        checkequal(["a", "b|c|d"], "a|b|c|d", "split", "|", 1)
        checkequal(["a", "b", "c|d"], "a|b|c|d", "split", "|", 2)
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "split", "|", 3)
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "split", "|", 4)
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "split", "|",
                        sys.maxint-2)
        checkequal(["a|b|c|d"], "a|b|c|d", "split", "|", 0)
        checkequal(["a", "", "b||c||d"], "a||b||c||d", "split", "|", 2)
        checkequal(["endcase ", ""], "endcase |", "split", "|")
        checkequal(["", " startcase"], "| startcase", "split", "|")
        checkequal(["", "bothcase", ""], "|bothcase|", "split", "|")
        checkequal(["a", "", "b\x00c\x00d"], "a\x00\x00b\x00c\x00d", "split", "\x00", 2)

        checkequal(["a"]*20, ("a|"*20)[:-1], "split", "|")
        checkequal(["a"]*15 +["a|a|a|a|a"],
                                   ("a|"*20)[:-1], "split", "|", 15)

        # by string
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "split", "//")
        checkequal(["a", "b//c//d"], "a//b//c//d", "split", "//", 1)
        checkequal(["a", "b", "c//d"], "a//b//c//d", "split", "//", 2)
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "split", "//", 3)
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "split", "//", 4)
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "split", "//",
                        sys.maxint-10)
        checkequal(["a//b//c//d"], "a//b//c//d", "split", "//", 0)
        checkequal(["a", "", "b////c////d"], "a////b////c////d", "split", "//", 2)
        checkequal(["endcase ", ""], "endcase test", "split", "test")
        checkequal(["", " begincase"], "test begincase", "split", "test")
        checkequal(["", " bothcase ", ""], "test bothcase test",
                        "split", "test")
        checkequal(["a", "bc"], "abbbc", "split", "bb")
        checkequal(["", ""], "aaa", "split", "aaa")
        checkequal(["aaa"], "aaa", "split", "aaa", 0)
        checkequal(["ab", "ab"], "abbaab", "split", "ba")
        checkequal(["aaaa"], "aaaa", "split", "aab")
        checkequal([""], "", "split", "aaa")
        checkequal(["aa"], "aa", "split", "aaa")
        checkequal(["A", "bobb"], "Abbobbbobb", "split", "bbobb")
        checkequal(["A", "B", ""], "AbbobbBbbobb", "split", "bbobb")

        checkequal(["a"]*20, ("aBLAH"*20)[:-4], "split", "BLAH")
        checkequal(["a"]*20, ("aBLAH"*20)[:-4], "split", "BLAH", 19)
        checkequal(["a"]*18 + ["aBLAHa"], ("aBLAH"*20)[:-4],
                        "split", "BLAH", 18)

    //test_rsplit
        checkequal(["this", "is", "the", "rsplit", "function"],
                         "this is the rsplit function", "rsplit")

        # by whitespace
        checkequal(["a", "b", "c", "d"], "a b c d ", "rsplit")
        checkequal(["a b c", "d"], "a b c d", "rsplit", None, 1)
        checkequal(["a b", "c", "d"], "a b c d", "rsplit", None, 2)
        checkequal(["a", "b", "c", "d"], "a b c d", "rsplit", None, 3)
        checkequal(["a", "b", "c", "d"], "a b c d", "rsplit", None, 4)
        checkequal(["a", "b", "c", "d"], "a b c d", "rsplit", None,
                        sys.maxint-20)
        checkequal(["a b c d"], "a b c d", "rsplit", None, 0)
        checkequal(["a b c d"], "a b c d  ", "rsplit", None, 0)
        checkequal(["a  b", "c", "d"], "a  b  c  d", "rsplit", None, 2)

        checkequal([], "         ", "rsplit")
        checkequal(["a"], "  a    ", "rsplit")
        checkequal(["a", "b"], "  a    b   ", "rsplit")
        checkequal(["  a", "b"], "  a    b   ", "rsplit", None, 1)
        checkequal(["  a    b","c"], "  a    b   c   ", "rsplit",
                        None, 1)
        checkequal(["  a", "b", "c"], "  a    b   c   ", "rsplit",
                        None, 2)
        checkequal(["a", "b"], "\n\ta \t\r b \v ", "rsplit", None, 88)
        aaa = " a "*20
        checkequal(["a"]*20, aaa, "rsplit")
        checkequal([aaa[:-4]] + ["a"], aaa, "rsplit", None, 1)
        checkequal([" a  a"] + ["a"]*18, aaa, "rsplit", None, 18)


        # by a char
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "rsplit", "|")
        checkequal(["a|b|c", "d"], "a|b|c|d", "rsplit", "|", 1)
        checkequal(["a|b", "c", "d"], "a|b|c|d", "rsplit", "|", 2)
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "rsplit", "|", 3)
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "rsplit", "|", 4)
        checkequal(["a", "b", "c", "d"], "a|b|c|d", "rsplit", "|",
                        sys.maxint-100)
        checkequal(["a|b|c|d"], "a|b|c|d", "rsplit", "|", 0)
        checkequal(["a||b||c", "", "d"], "a||b||c||d", "rsplit", "|", 2)
        checkequal(["", " begincase"], "| begincase", "rsplit", "|")
        checkequal(["endcase ", ""], "endcase |", "rsplit", "|")
        checkequal(["", "bothcase", ""], "|bothcase|", "rsplit", "|")

        checkequal(["a\x00\x00b", "c", "d"], "a\x00\x00b\x00c\x00d", "rsplit", "\x00", 2)

        checkequal(["a"]*20, ("a|"*20)[:-1], "rsplit", "|")
        checkequal(["a|a|a|a|a"]+["a"]*15,
                        ("a|"*20)[:-1], "rsplit", "|", 15)

        # by string
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "rsplit", "//")
        checkequal(["a//b//c", "d"], "a//b//c//d", "rsplit", "//", 1)
        checkequal(["a//b", "c", "d"], "a//b//c//d", "rsplit", "//", 2)
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "rsplit", "//", 3)
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "rsplit", "//", 4)
        checkequal(["a", "b", "c", "d"], "a//b//c//d", "rsplit", "//",
                        sys.maxint-5)
        checkequal(["a//b//c//d"], "a//b//c//d", "rsplit", "//", 0)
        checkequal(["a////b////c", "", "d"], "a////b////c////d", "rsplit", "//", 2)
        checkequal(["", " begincase"], "test begincase", "rsplit", "test")
        checkequal(["endcase ", ""], "endcase test", "rsplit", "test")
        checkequal(["", " bothcase ", ""], "test bothcase test",
                        "rsplit", "test")
        checkequal(["ab", "c"], "abbbc", "rsplit", "bb")
        checkequal(["", ""], "aaa", "rsplit", "aaa")
        checkequal(["aaa"], "aaa", "rsplit", "aaa", 0)
        checkequal(["ab", "ab"], "abbaab", "rsplit", "ba")
        checkequal(["aaaa"], "aaaa", "rsplit", "aab")
        checkequal([""], "", "rsplit", "aaa")
        checkequal(["aa"], "aa", "rsplit", "aaa")
        checkequal(["bbob", "A"], "bbobbbobbA", "rsplit", "bbobb")
        checkequal(["", "B", "A"], "bbobbBbbobbA", "rsplit", "bbobb")

        checkequal(["a"]*20, ("aBLAH"*20)[:-4], "rsplit", "BLAH")
        checkequal(["a"]*20, ("aBLAH"*20)[:-4], "rsplit", "BLAH", 19)
        checkequal(["aBLAHa"] + ["a"]*18, ("aBLAH"*20)[:-4],
                        "rsplit", "BLAH", 18)
*/
    //test_strip
    checkequal(string("hello"), string("   hello   ").strip());
    checkequal(string("hello   "), string("   hello   ").lstrip());
    checkequal(string("   hello"), string("   hello   ").rstrip());
    checkequal(string("hello"), string("hello").strip());

    //strip/lstrip/rstrip with str arg
    checkequal(string("hello"), string("xyzzyhelloxyzzy").strip("xyz"));
    checkequal(string("helloxyzzy"), string("xyzzyhelloxyzzy").lstrip("xyz"));
    checkequal(string("xyzzyhello"), string("xyzzyhelloxyzzy").rstrip("xyz"));
    checkequal(string("hello"), string("hello").strip("xyz"));

    //test_ljust
    checkequal(string("abc       "), string("abc").ljust(10));
    checkequal(string("abc   "), string("abc").ljust(6));
    checkequal(string("abc"), string("abc").ljust(3));
    checkequal(string("abc"), string("abc").ljust(2));
    checkequal(string("abc*******"), string("abc").ljust(10, '*'));

    //test_rjust
    checkequal(string("       abc"), string("abc").rjust(10));
    checkequal(string("   abc"), string("abc").rjust(6));
    checkequal(string("abc"), string("abc").rjust(3));
    checkequal(string("abc"), string("abc").rjust(2));
    checkequal(string("*******abc"), string("abc").rjust(10, '*'));

    //test_center
    checkequal(string("   abc    "), string("abc").center(10));
    checkequal(string(" abc  "), string("abc").center(6));
    checkequal(string("abc"), string("abc").center(3));
    checkequal(string("abc"), string("abc").center(2));
    checkequal(string("***abc****"), string("abc").center(10, '*'));

    //test_swapcase
    checkequal(string("hEllO CoMPuTErS"), string("HeLLo cOmpUteRs").swapcase());


    //test_replace
    // Operations on the empty string
    //checkequal("", string("").replace("", ""))
    //checkequal("A", string("").replace("", "A"))
    checkequal("", string("").replace("A", ""))
    checkequal("", string("").replace("A", "A"))
    //checkequal("", string("").replace("", "", 100))
    //checkequal("", string("").replace("", "", sys.maxint))

    // interleave (from=="", "to" gets inserted everywhere)
    //checkequal("A", string("A").replace("", ""))
    //checkequal("*A*", string("A").replace("", "*"))
    //checkequal("*1A*1", string("A").replace("", "*1"))
    //checkequal("*-#A*-#", string("A").replace("", "*-#"))
    //checkequal("*-A*-A*-", string("AA").replace("", "*-"))
    //checkequal("*-A*-A*-", string("AA").replace("", "*-", -1))
    //checkequal("*-A*-A*-", string("AA").replace("", "*-", sys.maxint))
    //checkequal("*-A*-A*-", string("AA").replace("", "*-", 4))
    //checkequal("*-A*-A*-", string("AA").replace("", "*-", 3))
    //checkequal("*-A*-A", string("AA").replace("", "*-", 2))
    //checkequal("*-AA", string("AA").replace("", "*-", 1))
    //checkequal("AA", string("AA").replace("", "*-", 0))

    // single character deletion (from=="A", to=="")
    checkequal("", string("A").replace("A", ""))
    checkequal("", string("AAA").replace("A", ""))
    checkequal("", string("AAA").replace("A", "", -1))
    //checkequal("", string("AAA").replace("A", "", sys.maxint))
    checkequal("", string("AAA").replace("A", "", 4))
    checkequal("", string("AAA").replace("A", "", 3))
    checkequal("A", string("AAA").replace("A", "", 2))
    checkequal("AA", string("AAA").replace("A", "", 1))
    checkequal("AAA", string("AAA").replace("A", "", 0))
    checkequal("", string("AAAAAAAAAA").replace("A", ""))
    checkequal("BCD", string("ABACADA").replace("A", ""))
    checkequal("BCD", string("ABACADA").replace("A", "", -1))
    //checkequal("BCD", string("ABACADA").replace("A", "", sys.maxint))
    checkequal("BCD", string("ABACADA").replace("A", "", 5))
    checkequal("BCD", string("ABACADA").replace("A", "", 4))
    checkequal("BCDA", string("ABACADA").replace("A", "", 3))
    checkequal("BCADA", string("ABACADA").replace("A", "", 2))
    checkequal("BACADA", string("ABACADA").replace("A", "", 1))
    checkequal("ABACADA", string("ABACADA").replace("A", "", 0))
    checkequal("BCD", string("ABCAD").replace("A", ""))
    checkequal("BCD", string("ABCADAA").replace("A", ""))
    checkequal("BCD", string("BCD").replace("A", ""))
    checkequal("*************", string("*************").replace("A", ""))
    checkequal("^A^", string(string("^")+string("A")*1000+string("^")).replace("A", "", 999))

    // substring deletion (from=="the", to=="")
    checkequal("", string("the").replace("the", ""))
    checkequal("ater", string("theater").replace("the", ""))
    checkequal("", string("thethe").replace("the", ""))
    checkequal("", string("thethethethe").replace("the", ""))
    checkequal("aaaa", string("theatheatheathea").replace("the", ""))
    checkequal("that", string("that").replace("the", ""))
    checkequal("thaet", string("thaet").replace("the", ""))
    checkequal("here and re", string("here and there").replace("the", ""))
    //checkequal("here and re and re", string("here and there and there").replace("the", "", sys.maxint))
    checkequal("here and re and re", string("here and there and there").replace("the", "", -1))
    checkequal("here and re and re", string("here and there and there").replace("the", "", 3))
    checkequal("here and re and re", string("here and there and there").replace("the", "", 2))
    checkequal("here and re and there", string("here and there and there").replace("the", "", 1))
    checkequal("here and there and there", string("here and there and there").replace("the", "", 0))
    checkequal("here and re and re", string("here and there and there").replace("the", ""))

    checkequal("abc", string("abc").replace("the", ""))
    checkequal("abcdefg", string("abcdefg").replace("the", ""))

    // substring deletion (from=="bob", to=="")
    checkequal("bob", string("bbobob").replace("bob", ""))
    checkequal("bobXbob", string("bbobobXbbobob").replace("bob", ""))
    checkequal("aaaaaaa", string("aaaaaaabob").replace("bob", ""))
    checkequal("aaaaaaa", string("aaaaaaa").replace("bob", ""))

    // single character replace in place (len(from)==len(to)==1)
    checkequal("Who goes there?", string("Who goes there?").replace("o", "o"))
    checkequal("WhO gOes there?", string("Who goes there?").replace("o", "O"))
    //checkequal("WhO gOes there?", string("Who goes there?").replace("o", "O", sys.maxint))
    checkequal("WhO gOes there?", string("Who goes there?").replace("o", "O", -1))
    checkequal("WhO gOes there?", string("Who goes there?").replace("o", "O", 3))
    checkequal("WhO gOes there?", string("Who goes there?").replace("o", "O", 2))
    checkequal("WhO goes there?", string("Who goes there?").replace("o", "O", 1))
    checkequal("Who goes there?", string("Who goes there?").replace("o", "O", 0))

    checkequal("Who goes there?", string("Who goes there?").replace("a", "q"))
    checkequal("who goes there?", string("Who goes there?").replace("W", "w"))
    checkequal("wwho goes there?ww", string("WWho goes there?WW").replace("W", "w"))
    checkequal("Who goes there!", string("Who goes there?").replace("?", "!"))
    checkequal("Who goes there!!", string("Who goes there??").replace("?", "!"))

    checkequal("Who goes there?", string("Who goes there?").replace(".", "!"))

    // substring replace in place (len(from)==len(to) > 1)
    checkequal("Th** ** a t**sue", string("This is a tissue").replace("is", "**"))
    //checkequal("Th** ** a t**sue", string("This is a tissue").replace("is", "**", sys.maxint))
    checkequal("Th** ** a t**sue", string("This is a tissue").replace("is", "**", -1))
    checkequal("Th** ** a t**sue", string("This is a tissue").replace("is", "**", 4))
    checkequal("Th** ** a t**sue", string("This is a tissue").replace("is", "**", 3))
    checkequal("Th** ** a tissue", string("This is a tissue").replace("is", "**", 2))
    checkequal("Th** is a tissue", string("This is a tissue").replace("is", "**", 1))
    checkequal("This is a tissue", string("This is a tissue").replace("is", "**", 0))
    checkequal("cobob", string("bobob").replace("bob", "cob"))
    checkequal("cobobXcobocob", string("bobobXbobobob").replace("bob", "cob"))
    checkequal("bobob", string("bobob").replace("bot", "bot"))

    // replace single character (len(from)==1, len(to)>1)
    checkequal("ReyKKjaviKK", string("Reykjavik").replace("k", "KK"))
    checkequal("ReyKKjaviKK", string("Reykjavik").replace("k", "KK", -1))
    //checkequal("ReyKKjaviKK", string("Reykjavik").replace("k", "KK", sys.maxint))
    checkequal("ReyKKjaviKK", string("Reykjavik").replace("k", "KK", 2))
    checkequal("ReyKKjavik", string("Reykjavik").replace("k", "KK", 1))
    checkequal("Reykjavik", string("Reykjavik").replace("k", "KK", 0))
    checkequal("A----B----C----", string("A.B.C.").replace(".", "----"))

    checkequal("Reykjavik", string("Reykjavik").replace("q", "KK"))

    // replace substring (len(from)>1, len(to)!=len(from))
    checkequal("ham, ham, eggs and ham", string("spam, spam, eggs and spam").replace("spam", "ham"))
    //checkequal("ham, ham, eggs and ham", string("spam, spam, eggs and spam").replace("spam", "ham", sys.maxint))
    checkequal("ham, ham, eggs and ham", string("spam, spam, eggs and spam").replace("spam", "ham", -1))
    checkequal("ham, ham, eggs and ham", string("spam, spam, eggs and spam").replace("spam", "ham", 4))
    checkequal("ham, ham, eggs and ham", string("spam, spam, eggs and spam").replace("spam", "ham", 3))
    checkequal("ham, ham, eggs and spam", string("spam, spam, eggs and spam").replace("spam", "ham", 2))
    checkequal("ham, spam, eggs and spam", string("spam, spam, eggs and spam").replace("spam", "ham", 1))
    checkequal("spam, spam, eggs and spam", string("spam, spam, eggs and spam").replace("spam", "ham", 0))

    checkequal("bobob", string("bobobob").replace("bobob", "bob"))
    checkequal("bobobXbobob", string("bobobobXbobobob").replace("bobob", "bob"))
    checkequal("BOBOBOB", string("BOBOBOB").replace("bob", "bobby"))

    checkequal("one@two!three!", string("one!two!three!").replace("!", "@", 1))
    checkequal("onetwothree", string("one!two!three!").replace("!", ""))
    checkequal("one@two@three!", string("one!two!three!").replace("!", "@", 2))
    checkequal("one@two@three@", string("one!two!three!").replace("!", "@", 3))
    checkequal("one@two@three@", string("one!two!three!").replace("!", "@", 4))
    checkequal("one!two!three!", string("one!two!three!").replace("!", "@", 0))
    checkequal("one@two@three@", string("one!two!three!").replace("!", "@"))
    checkequal("one!two!three!", string("one!two!three!").replace("x", "@"))
    checkequal("one!two!three!", string("one!two!three!").replace("x", "@", 2))
    //checkequal("-a-b-c-", string("abc").replace("", "-"))
    //checkequal("-a-b-c", string("abc").replace("", "-", 3))
    checkequal("abc", string("abc").replace("", "-", 0))
    checkequal("", string("").replace("", ""))
    checkequal("abc", string("abc").replace("ab", "--", 0))
    checkequal("abc", string("abc").replace("xy", "--"))
    // Next three for SF bug 422088: [OSF1 alpha] string.replace(); died with
    // MemoryError due to empty result (platform malloc issue when requesting
    // 0 bytes).
    checkequal("", string("123").replace("123", ""))
    checkequal("", string("123123").replace("123", ""))
    checkequal("x", string("123x123").replace("123", ""))

    //test_zfill
    checkequal(string("123"), string("123").zfill(2));
    checkequal(string("123"), string("123").zfill(3));
    checkequal(string("0123"), string("123").zfill(4));
    checkequal(string("+123"), string("+123").zfill(3));
    checkequal(string("+123"), string("+123").zfill(4));
    checkequal(string("+0123"), string("+123").zfill(5));
    checkequal(string("-123"), string("-123").zfill(3));
    checkequal(string("-123"), string("-123").zfill(4));
    checkequal(string("-0123"), string("-123").zfill(5));
    checkequal(string("000"), string("").zfill(3));
    checkequal(string("34"), string("34").zfill(1));
    checkequal(string("0034"), string("34").zfill(4));

    //test_islower
    checkequal(false, string("").islower());
    checkequal(true, string("a").islower());
    checkequal(false, string("A").islower());
    checkequal(false, string("\n").islower());
    checkequal(true, string("abc").islower());
    checkequal(false, string("aBc").islower());
    checkequal(true, string("abc\n").islower());

    //test_isupper
    checkequal(false, string("").isupper());
    checkequal(false, string("a").isupper());
    checkequal(true, string("A").isupper());
    checkequal(false, string("\n").isupper());
    checkequal(true, string("ABC").isupper());
    checkequal(false, string("AbC").isupper());
    checkequal(true, string("ABC\n").isupper());

    //test_istitle
    checkequal(false, string("").istitle());
    checkequal(false, string("a").istitle());
    checkequal(true, string("A").istitle());
    checkequal(false, string("\n").istitle());
    checkequal(true, string("A Titlecased Line").istitle());
    checkequal(true, string("A\nTitlecased Line").istitle());
    checkequal(true, string("A Titlecased, Line").istitle());
    checkequal(false, string("Not a capitalized String").istitle());
    checkequal(false, string("Not\ta Titlecase String").istitle());
    checkequal(false, string("Not--a Titlecase String").istitle());
    checkequal(false, string("NOT").istitle());

    //test_isspace
    checkequal(false, string("").isspace());
    checkequal(false, string("a").isspace());
    checkequal(true, string(" ").isspace());
    checkequal(true, string("\t").isspace());
    checkequal(true, string("\r").isspace());
    checkequal(true, string("\n").isspace());
    checkequal(true, string(" \t\r\n").isspace());
    checkequal(false, string(" \t\r\na").isspace());

    //test_isalpha
    checkequal(false, string("").isalpha());
    checkequal(true, string("a").isalpha());
    checkequal(true, string("A").isalpha());
    checkequal(false, string("\n").isalpha());
    checkequal(true, string("abc").isalpha());
    checkequal(false, string("aBc123").isalpha());
    checkequal(false, string("abc\n").isalpha());

    //test_isalnum
    checkequal(false, string("").isalnum());
    checkequal(true, string("a").isalnum());
    checkequal(true, string("A").isalnum());
    checkequal(false, string("\n").isalnum());
    checkequal(true, string("123abc456").isalnum());
    checkequal(true, string("a1b3c").isalnum());
    checkequal(false, string("aBc000 ").isalnum());
    checkequal(false, string("abc\n").isalnum());

    //test_isdigit
    checkequal(false, string("").isdigit());
    checkequal(false, string("a").isdigit());
    checkequal(true, string("0").isdigit());
    checkequal(true, string("0123456789").isdigit());
    checkequal(false, string("0123456789a").isdigit());

    //test_title
    checkequal(string(" Hello "), string(" hello ").title());
    checkequal(string("Hello "), string("hello ").title());
    checkequal(string("Hello "), string("Hello ").title());
    checkequal(string("Format This As Title String"), string("fOrMaT thIs aS titLe String").title());
    checkequal(string("Format,This-As*Title;String"), string("fOrMaT,thIs-aS*titLe;String").title());
    checkequal(string("Getint"), string("getInt").title());
/*
    //test_splitlines
        checkequal(["abc", "def", "", "ghi"], "abc\ndef\n\rghi", "splitlines")
        checkequal(["abc", "def", "", "ghi"], "abc\ndef\n\r\nghi", "splitlines")
        checkequal(["abc", "def", "ghi"], "abc\ndef\r\nghi", "splitlines")
        checkequal(["abc", "def", "ghi"], "abc\ndef\r\nghi\n", "splitlines")
        checkequal(["abc", "def", "ghi", ""], "abc\ndef\r\nghi\n\r", "splitlines")
        checkequal(["", "abc", "def", "ghi", ""], "\nabc\ndef\r\nghi\n\r", "splitlines")
        checkequal(["\n", "abc\n", "def\r\n", "ghi\n", "\r"], "\nabc\ndef\r\nghi\n\r", "splitlines", 1)

        checkraises(TypeError, "abc", "splitlines", 42, 42)
*/
    //test_startswith
    checkequal(true, string("hello").startswith("he"));
    checkequal(true, string("hello").startswith("hello"));
    checkequal(false, string("hello").startswith("hello world"));
    checkequal(true, string("hello").startswith(""));
    checkequal(false, string("hello").startswith("ello"));
    checkequal(true, string("hello").startswith('h'));
    checkequal(false, string("hello").startswith('o'));
    //checkequal(true, string("hello").startswith("ello", 1));
    //checkequal(true, string("hello").startswith("o", 4));
    //checkequal(false, string("hello").startswith("o", 5));
    //checkequal(true, string("hello").startswith("", 5));
    //checkequal(false, string("hello").startswith("lo", 6));
    //checkequal(true, string("helloworld").startswith("lowo", 3));
    //checkequal(true, string("helloworld").startswith("lowo", 3, 7));
    //checkequal(false, string("helloworld").startswith("lowo", 3, 6));

    //test negative indices
    //checkequal(true, string("hello").startswith("he", 0, -1));
    //checkequal(true, string("hello").startswith("he", -53, -1));
    //checkequal(false, string("hello").startswith("hello", 0, -1));
    //checkequal(false, string("hello").startswith("hello world", -1, -10));
    //checkequal(false, string("hello").startswith("ello", -5));
    //checkequal(true, string("hello").startswith("ello", -4));
    //checkequal(false, string("hello").startswith("o", -2));
    //checkequal(true, string("hello").startswith("o", -1));
    //checkequal(true, string("hello").startswith("", -3, -3));
    //checkequal(false, string("hello").startswith("lo", -9));

    //test_endswith
    checkequal(true, string("hello").endswith("lo"));
    checkequal(false, string("hello").endswith("he"));
    checkequal(true, string("hello").endswith(""));
    checkequal(false, string("hello").endswith("hello world"));
    checkequal(false, string("helloworld").endswith("worl"));
    checkequal(true, string("hello").endswith('o'));
    checkequal(false, string("hello").endswith('h'));
    checkequal(false, string("hello").endswith('\0'));
    //checkequal(true, string("helloworld").endswith("worl", 3, 9));
    //checkequal(true, string("helloworld").endswith("world", 3, 12));
    //checkequal(true, string("helloworld").endswith("lowo", 1, 7));
    //checkequal(true, string("helloworld").endswith("lowo", 2, 7));
    //checkequal(true, string("helloworld").endswith("lowo", 3, 7));
    //checkequal(false, string("helloworld").endswith("lowo", 4, 7));
    //checkequal(false, string("helloworld").endswith("lowo", 3, 8));
    //checkequal(false, string("ab").endswith("ab", 0, 1));
    //checkequal(false, string("ab").endswith("ab", 0, 0));

    //test negative indices
    //checkequal(true, string("hello").endswith("lo", -2));
    //checkequal(false, string("hello").endswith("he", -2));
    //checkequal(true, string("hello").endswith("", -3, -3));
    //checkequal(false, string("hello").endswith("hello world", -10, -2));
    //checkequal(false, string("helloworld").endswith("worl", -6));
    //checkequal(true, string("helloworld").endswith("worl", -5, -1));
    //checkequal(true, string("helloworld").endswith("worl", -5, 9));
    //checkequal(true, string("helloworld").endswith("world", -7, 12));
    //checkequal(true, string("helloworld").endswith("lowo", -99, -3));
    //checkequal(true, string("helloworld").endswith("lowo", -8, -3));
    //checkequal(true, string("helloworld").endswith("lowo", -7, -3));
    //checkequal(false, string("helloworld").endswith("lowo", 3, -4));
    //checkequal(false, string("helloworld").endswith("lowo", -8, -2));
/*
    //test_join
        # join now works with any sequence type
        # moved here, because the argument order is
        # different in string.join (see the test in
        # test.test_string.StringTest.test_join)
        checkequal("a b c d", " ", "join", ["a", "b", "c", "d"])
        checkequal("abcd", "", "join", ("a", "b", "c", "d"))
        checkequal("bd", "", "join", ("", "b", "", "d"))
        checkequal("ac", "", "join", ("a", "", "c", ""))
        checkequal("w x y z", " ", "join", Sequence())
        checkequal("abc", "a", "join", ("abc",))
        checkequal("z", "a", "join", UserList(["z"]))
        if test_support.have_unicode:
            checkequal(unicode("a.b.c"), unicode("."), "join", ["a", "b", "c"])
            checkequal(unicode("a.b.c"), ".", "join", [unicode("a"), "b", "c"])
            checkequal(unicode("a.b.c"), ".", "join", ["a", unicode("b"), "c"])
            checkequal(unicode("a.b.c"), ".", "join", ["a", "b", unicode("c")])
            checkraises(TypeError, ".", "join", ["a", unicode("b"), 3])
        for i in [5, 25, 125]:
            checkequal(((("a" * i) + "-") * i)[:-1], "-", "join",
                 ["a" * i] * i)
            checkequal(((("a" * i) + "-") * i)[:-1], "-", "join",
                 ("a" * i,) * i)

        checkraises(TypeError, " ", "join", BadSeq1())
        checkequal("a b c", " ", "join", BadSeq2())

        checkraises(TypeError, " ", "join")
        checkraises(TypeError, " ", "join", 7)
        checkraises(TypeError, " ", "join", Sequence([7, "hello", 123L]))
        try:
            //f():
                yield 4 + ""
            fixtype(" ").join(f())
        except TypeError, e:
            if "+" not in str(e):
                fail("join() ate exception message")
        else:
            fail("exception not raised")

    //test_formatting
        checkequal("+hello+", "+%s+", "__mod__", "hello")
        checkequal("+10+", "+%d+", "__mod__", 10)
        checkequal("a", "%c", "__mod__", "a")
        checkequal("a", "%c", "__mod__", "a")
        checkequal(""", "%c", "__mod__", 34)
        checkequal("$", "%c", "__mod__", 36)
        checkequal("10", "%d", "__mod__", 10)
        checkequal("\x7f", "%c", "__mod__", 0x7f)

        for ordinal in (-100, 0x200000):
            # unicode raises ValueError, str raises OverflowError
            checkraises((ValueError, OverflowError), "%c", "__mod__", ordinal)

        longvalue = sys.maxint + 10L
        slongvalue = str(longvalue)
        if slongvalue[-1] in ("L","l"): slongvalue = slongvalue[:-1]
        checkequal(" 42", "%3ld", "__mod__", 42)
        checkequal("42", "%d", "__mod__", 42L)
        checkequal("42", "%d", "__mod__", 42.0)
        checkequal(slongvalue, "%d", "__mod__", longvalue)
        checkcall("%d", "__mod__", float(longvalue))
        checkequal("0042.00", "%07.2f", "__mod__", 42)
        checkequal("0042.00", "%07.2F", "__mod__", 42)

        checkraises(TypeError, "abc", "__mod__")
        checkraises(TypeError, "%(foo)s", "__mod__", 42)
        checkraises(TypeError, "%s%s", "__mod__", (42,))
        checkraises(TypeError, "%c", "__mod__", (None,))
        checkraises(ValueError, "%(foo", "__mod__", {})
        checkraises(TypeError, "%(foo)s %(bar)s", "__mod__", ("foo", 42))
        checkraises(TypeError, "%d", "__mod__", "42") # not numeric
        checkraises(TypeError, "%d", "__mod__", (42+0j)) # no int/long conversion provided

        # argument names with properly nested brackets are supported
        checkequal("bar", "%((foo))s", "__mod__", {"(foo)": "bar"})

        # 100 is a magic number in PyUnicode_Format, this forces a resize
        checkequal(103*"a"+"x", "%sx", "__mod__", 103*"a")

        checkraises(TypeError, "%*s", "__mod__", ("foo", "bar"))
        checkraises(TypeError, "%10.*f", "__mod__", ("foo", 42.))
        checkraises(ValueError, "%10", "__mod__", (42,))

    //test_floatformatting
        # float formatting
        for prec in xrange(100):
            format = "%%.%if" % prec
            value = 0.01
            for x in xrange(60):
                value = value * 3.14159265359 / 3.0 * 10.0
                checkcall(format, "__mod__", value)

    //test_partition

        checkequal(("this is the par", "ti", "tion method"),
            "this is the partition method", "partition", "ti")

        # from raymond"s original specification
        S = "http://www.python.org"
        checkequal(("http", "://", "www.python.org"), S, "partition", "://")
        checkequal(("http://www.python.org", "", ""), S, "partition", "?")
        checkequal(("", "http://", "www.python.org"), S, "partition", "http://")
        checkequal(("http://www.python.", "org", ""), S, "partition", "org")

    //test_rpartition

        checkequal(("this is the rparti", "ti", "on method"),
            "this is the rpartition method", "rpartition", "ti")

        # from raymond"s original specification
        S = "http://www.python.org"
        checkequal(("http", "://", "www.python.org"), S, "rpartition", "://")
        checkequal(("", "", "http://www.python.org"), S, "rpartition", "?")
        checkequal(("", "http://", "www.python.org"), S, "rpartition", "http://")
        checkequal(("http://www.python.", "org", ""), S, "rpartition", "org")
*/
    if (error_count) exit(1);
}
