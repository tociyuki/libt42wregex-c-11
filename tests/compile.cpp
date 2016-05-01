#include <vector>
#include <string>
#include <iostream>
#include <locale>
#include <utility>
#include "t42wregex.hpp"
#include "wtaptests.hpp"

std::wstring esc (std::wstring const& s)
{
    std::wstring t;
    for (auto c : s) {
        if (L'"' == c)
            t += L"\\\"";
        else if (0x20 <= c && c < 0x7f)
            t.push_back (c);
        else if (L'\t' == c)
            t += L"\\t";
        else if (L'\a' == c)
            t += L"\\a";
        else if (L'\f' == c)
            t += L"\\f";
        else if (L'\r' == c)
            t += L"\\r";
        else if (L'\n' == c)
            t += L"\\n";
        else if (L'\v' == c)
            t += L"\\v";
        else if (0 <= c && c < 256) {
            unsigned int hi = (c >> 4) & 0x0f;
            unsigned int lo = c & 0x0f;
            t += L"\\x";
            t.push_back (hi < 10 ? hi + L'0' : hi + L'a' - 10);
            t.push_back (lo < 10 ? lo + L'0' : lo + L'a' - 10);
        }
    }
    return std::move (t);
}

std::wstring esc (wchar_t c)
{
    std::wstring s (1, c);
    return esc (s);
}

std::wstring list (std::wstring const& s)
{
    std::wstring t;
    t42::wregex re (s);
    t42::wpike::program e = re.prog ();
    for (auto op : e)
        switch (op.opcode) {
        case t42::wpike::MATCH:
            t += L"match\n";
            break;
        case t42::wpike::SAVE:
            t += L"save ";
            t += std::to_wstring (op.x);
            t += L"\n";
            break;
        case t42::wpike::CHAR:
            t += L"char '";
            t += esc (op.s);
            t += L"'\n";
            break;
        case t42::wpike::ANY:
            t += L"any\n";
            break;
        case t42::wpike::CCLASS:
            t += L"cclass \"";
            t += esc (op.s);
            t += L"\"\n";
            break;
        case t42::wpike::NCCLASS:
            t += L"ncclass \"";
            t += esc (op.s);
            t += L"\"\n";
            break;
        case t42::wpike::BKREF:
            t += L"bkref ";
            t += std::to_wstring (op.x);
            t += L",%";
            t += std::to_wstring (op.r);
            t += L"\n";
            break;
        case t42::wpike::BOL:
            t += L"bol\n";
            break;
        case t42::wpike::EOL:
            t += L"eol\n";
            break;
        case t42::wpike::BOS:
            t += L"bos\n";
            break;
        case t42::wpike::EOS:
            t += L"eos\n";
            break;
        case t42::wpike::WORDB:
            t += L"wordb\n";
            break;
        case t42::wpike::NWORDB:
            t += L"nwordb\n";
            break;
        case t42::wpike::JMP:
            t += L"jmp ";
            t += std::to_wstring (op.x);
            t += L"\n";
            break;
        case t42::wpike::SPLIT:
            t += L"split ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L"\n";
            break;
        case t42::wpike::LKAHEAD:
            t += L"lkahead ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L"\n";
            break;
        case t42::wpike::NLKAHEAD:
            t += L"nlkahead ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L"\n";
            break;
        case t42::wpike::LKBEHIND:
            t += L"lkbehind ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L"\n";
            break;
        case t42::wpike::NLKBEHIND:
            t += L"nlkbehind ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L"\n";
            break;
        case t42::wpike::RESET:
            t += L"reset %";
            t += std::to_wstring (op.r);
            t += L"\n";
            break;
        case t42::wpike::REP:
            t += L"rep ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L",%";
            t += std::to_wstring (op.r);
            t += L"\n";
            break;
        case t42::wpike::INCJMP:
            t += L"incjmp ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L",%";
            t += std::to_wstring (op.r);
            t += L"\n";
            break;
        case t42::wpike::DECJMP:
            t += L"decjmp ";
            t += std::to_wstring (op.x);
            t += L",";
            t += std::to_wstring (op.y);
            t += L",%";
            t += std::to_wstring (op.r);
            t += L"\n";
            break;
        default:
            t += L"?\n";
        }
    return std::move (t);
}

struct testspec {
    wchar_t const* input;
    wchar_t const* expected;
} spec[]{
    {L"a|b",
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp 1\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(?:a|b)",
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp 1\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(a|b)",
     L"save 2\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp 1\n"
     L"char 'b'\n"
     L"save 3\n"
     L"match\n"},

    {L"a|b|c",
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp 4\n"
     L"split 0,2\n"
     L"char 'b'\n"
     L"jmp 1\n"
     L"char 'c'\n"
     L"match\n"},

    {L"(?:ab|cd|efg)",
     L"split 0,3\n"
     L"char 'a'\n"
     L"char 'b'\n"
     L"jmp 7\n"
     L"split 0,3\n"
     L"char 'c'\n"
     L"char 'd'\n"
     L"jmp 3\n"
     L"char 'e'\n"
     L"char 'f'\n"
     L"char 'g'\n"
     L"match\n"},

    {L"(ab|cd|efg)",
     L"save 2\n"
     L"split 0,3\n"
     L"char 'a'\n"
     L"char 'b'\n"
     L"jmp 7\n"
     L"split 0,3\n"
     L"char 'c'\n"
     L"char 'd'\n"
     L"jmp 3\n"
     L"char 'e'\n"
     L"char 'f'\n"
     L"char 'g'\n"
     L"save 3\n"
     L"match\n"},

    {L"a|",
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp 0\n"
     L"match\n"},

    {L"|b",
     L"split 0,1\n"
     L"jmp 1\n"
     L"char 'b'\n"
     L"match\n"},

    {L"a?",
     L"split 0,1\n"
     L"char 'a'\n"
     L"match\n"},

    {L"(?:ab)?",
     L"split 0,2\n"
     L"char 'a'\n"
     L"char 'b'\n"
     L"match\n"},

    {L"a??",
     L"split 1,0\n"
     L"char 'a'\n"
     L"match\n"},

    {L"(?:ab)??",
     L"split 2,0\n"
     L"char 'a'\n"
     L"char 'b'\n"
     L"match\n"},

    {L"a*",
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"(?:ab)*",
     L"split 0,3\n"
     L"char 'a'\n"
     L"char 'b'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a*?",
     L"split 2,0\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"(?:ab)*?",
     L"split 3,0\n"
     L"char 'a'\n"
     L"char 'b'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a+",
     L"char 'a'\n"
     L"split -2,0\n"
     L"match\n"},

    {L"(?:ab)+",
     L"char 'a'\n"
     L"char 'b'\n"
     L"split -3,0\n"
     L"match\n"},

    {L"a+?",
     L"char 'a'\n"
     L"split 0,-2\n"
     L"match\n"},

    {L"(?:ab)+?",
     L"char 'a'\n"
     L"char 'b'\n"
     L"split 0,-3\n"
     L"match\n"},

    {L"a{0,3}",
     L"reset %0\n"
     L"rep 0,3,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{0,3}?",
     L"reset %0\n"
     L"rep 0,3,%0\n"
     L"split 2,0\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{1,3}",
     L"reset %0\n"
     L"rep 1,3,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{1,3}?",
     L"reset %0\n"
     L"rep 1,3,%0\n"
     L"split 2,0\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{2}",
     L"reset %0\n"
     L"rep 2,2,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{2}?",
     L"reset %0\n"
     L"rep 2,2,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{2,4}",
     L"reset %0\n"
     L"rep 2,4,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{2,4}?",
     L"reset %0\n"
     L"rep 2,4,%0\n"
     L"split 2,0\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{2,}",
     L"reset %0\n"
     L"rep 2,-1,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"a{2,}?",
     L"reset %0\n"
     L"rep 2,-1,%0\n"
     L"split 2,0\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"match\n"},

    {L"q{a}",
     L"char 'q'\n"
     L"char '{'\n"
     L"char 'a'\n"
     L"char '}'\n"
     L"match\n"},

    {L"(a(b)c)d(e)",
     L"save 2\n"
     L"char 'a'\n"
     L"save 4\n"
     L"char 'b'\n"
     L"save 5\n"
     L"char 'c'\n"
     L"save 3\n"
     L"char 'd'\n"
     L"save 6\n"
     L"char 'e'\n"
     L"save 7\n"
     L"match\n"},

    {L"a",
     L"char 'a'\n"
     L"match\n"},

    {L".",
     L"any\n"
     L"match\n"},

    {L"^",
     L"bol\n"
     L"match\n"},

    {L"$",
     L"eol\n"
     L"match\n"},

    {L"\\A",
     L"bos\n"
     L"match\n"},

    {L"\\z",
     L"eos\n"
     L"match\n"},

    {L"\\b",
     L"wordb\n"
     L"match\n"},

    {L"\\B",
     L"nwordb\n"
     L"match\n"},

    {L"\\a",
     L"char '\\a'\n"
     L"match\n"},

    {L"\\f",
     L"char '\\f'\n"
     L"match\n"},

    {L"\\n",
     L"char '\\n'\n"
     L"match\n"},

    {L"\\r",
     L"char '\\r'\n"
     L"match\n"},

    {L"\\v",
     L"char '\\v'\n"
     L"match\n"},

    {L"\\0a",
     L"char '\\x00'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\1a",
     L"reset %0\n"
     L"bkref 1,%0\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\12a",
     L"char '\\n'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\91",
     L"reset %0\n"
     L"bkref 9,%0\n"
     L"char '1'\n"
     L"match\n"},

    {L"\\12a",
     L"char '\\n'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\012a",
     L"char '\\n'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\00123",
     L"char '\\n'\n"
     L"char '3'\n"
     L"match\n"},

    {L"\\x0g",
     L"char '\\x00'\n"
     L"char 'g'\n"
     L"match\n"},

    {L"\\x01g",
     L"char '\\x01'\n"
     L"char 'g'\n"
     L"match\n"},

    {L"\\x012",
     L"char '\\x01'\n"
     L"char '2'\n"
     L"match\n"},

    {L"\\x30\\x31\\x32",
     L"char '0'\n"
     L"char '1'\n"
     L"char '2'\n"
     L"match\n"},

    {L"\\x{0}1",
     L"char '\\x00'\n"
     L"char '1'\n"
     L"match\n"},

    {L"\\x{00000030}1",
     L"char '0'\n"
     L"char '1'\n"
     L"match\n"},

    {L"\\cjk",
     L"char '\\n'\n"
     L"char 'k'\n"
     L"match\n"},

    {L"\\u00301",
     L"char '0'\n"
     L"char '1'\n"
     L"match\n"},

    {L"\\U000000301",
     L"char '0'\n"
     L"char '1'\n"
     L"match\n"},

    {L"[a]",
     L"cclass \"\\a\"\n"
     L"match\n"},

    {L"[abc]",
     L"cclass \"\\a\\b\\c\"\n"
     L"match\n"},

    {L"[a-z]",
     L"cclass \"\\a-\\z\"\n"
     L"match\n"},

    {L"[\\x30-\\x39]",
     L"cclass \"\\0-\\9\"\n"
     L"match\n"},

    {L"[a\\]\\-z]",
     L"cclass \"\\a\\]\\-\\z\"\n"
     L"match\n"},

    {L"[ab-fg]",
     L"cclass \"\\a\\b-\\f\\g\"\n"
     L"match\n"},

    {L"[$^.?*+\\-()]",
     L"cclass \"\\$\\^\\.\\?\\*\\+\\-\\(\\)\"\n"
     L"match\n"},

    {L"[-]",
     L"cclass \"\\-\"\n"
     L"match\n"},

    {L"[-a]",
     L"cclass \"\\-\\a\"\n"
     L"match\n"},

    {L"[a-]",
     L"cclass \"\\a\\-\"\n"
     L"match\n"},

    {L"[-a-]",
     L"cclass \"\\-\\a\\-\"\n"
     L"match\n"},

    {L"[-a-z-]",
     L"cclass \"\\-\\a-\\z\\-\"\n"
     L"match\n"},

    /*
     * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html
     *
     * 9.3.5 RE Bracket Expression
     *
     *   7. the expression "[%--]" matches
     *      any of the characters between '%' and '-'
     */
    {L"[%--]",
     L"cclass \"\\%-\\-\"\n"
     L"match\n"},

    {L"[[:alnum:][:alpha:][:blank:][:cntrl:]]",
     L"cclass \":a:b:c:d\"\n"
     L"match\n"},

    {L"[[:digit:][:graph:][:lower:][:print:]]",
     L"cclass \":e:f:g:h\"\n"
     L"match\n"},

    {L"[[:space:][:upper:][:xdigit:][:word:]]",
     L"cclass \":i:j:k:l\"\n"
     L"match\n"},

    {L"[[:d:][:s:][:w:]]",
     L"cclass \":e:i:l\"\n"
     L"match\n"},

    {L"[[:^alnum:][:^alpha:][:^blank:][:^cntrl:]]",
     L"cclass \":m:n:o:p\"\n"
     L"match\n"},

    {L"[[:^digit:][:^graph:][:^lower:][:^print:]]",
     L"cclass \":q:r:s:t\"\n"
     L"match\n"},

    {L"[[:^space:][:^upper:][:^xdigit:][:^word:]]",
     L"cclass \":u:v:w:x\"\n"
     L"match\n"},

    {L"[[:^d:][:^s:][:^w:]]",
     L"cclass \":q:u:x\"\n"
     L"match\n"},

    {L"[[]",
     L"cclass \"\\[\"\n"
     L"match\n"},

    {L"[]]",
     L"cclass \"\\]\"\n"
     L"match\n"},

    {L"[^a]",
     L"ncclass \"\\a\"\n"
     L"match\n"},

    {L"[^abc]",
     L"ncclass \"\\a\\b\\c\"\n"
     L"match\n"},

    {L"[^a-z]",
     L"ncclass \"\\a-\\z\"\n"
     L"match\n"},

    {L"[^ab-fg]",
     L"ncclass \"\\a\\b-\\f\\g\"\n"
     L"match\n"},

    {L"[^$^.?*+\\-()]",
     L"ncclass \"\\$\\^\\.\\?\\*\\+\\-\\(\\)\"\n"
     L"match\n"},

    {L"[^-]",
     L"ncclass \"\\-\"\n"
     L"match\n"},

    {L"[^-a]",
     L"ncclass \"\\-\\a\"\n"
     L"match\n"},

    {L"[^a-]",
     L"ncclass \"\\a\\-\"\n"
     L"match\n"},

    {L"[^-a-]",
     L"ncclass \"\\-\\a\\-\"\n"
     L"match\n"},

    {L"[^-a-z-]",
     L"ncclass \"\\-\\a-\\z\\-\"\n"
     L"match\n"},

    {L"[^[]",
     L"ncclass \"\\[\"\n"
     L"match\n"},

    {L"[^]",
     L"cclass \"\\^\"\n"
     L"match\n"},

    {L"a(?=b).",
     L"char 'a'\n"
     L"lkahead 0,2\n"
     L"char 'b'\n"
     L"match\n"
     L"any\n"
     L"match\n"},

    {L"a(?!b).",
     L"char 'a'\n"
     L"nlkahead 0,2\n"
     L"char 'b'\n"
     L"match\n"
     L"any\n"
     L"match\n"},

    {L"(?<=a)b",
     L"lkbehind 0,2\n"
     L"char 'a'\n"
     L"match\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(?<=a+)b",
     L"lkbehind 0,3\n"
     L"char 'a'\n"
     L"split -2,0\n"
     L"match\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(?<=xyz)b",
     L"lkbehind 0,4\n"
     L"char 'z'\n"
     L"char 'y'\n"
     L"char 'x'\n"
     L"match\n"
     L"char 'b'\n"
     L"match\n"},

    {L"((?<=(a))b)",
     L"save 2\n"
     L"lkbehind 0,4\n"
     L"save 5\n"
     L"char 'a'\n"
     L"save 4\n"
     L"match\n"
     L"char 'b'\n"
     L"save 3\n"
     L"match\n"},

    {L"(?<!a)b",
     L"nlkbehind 0,2\n"
     L"char 'a'\n"
     L"match\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(?<!a+)b",
     L"nlkbehind 0,3\n"
     L"char 'a'\n"
     L"split -2,0\n"
     L"match\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(?<!xyz)b",
     L"nlkbehind 0,4\n"
     L"char 'z'\n"
     L"char 'y'\n"
     L"char 'x'\n"
     L"match\n"
     L"char 'b'\n"
     L"match\n"},

    {L".*?((?<=(a)(b))c)",
     L"split 2,0\n"
     L"any\n"
     L"jmp -3\n"
     L"save 2\n"
     L"lkbehind 0,7\n"
     L"save 7\n"
     L"char 'b'\n"
     L"save 6\n"
     L"save 5\n"
     L"char 'a'\n"
     L"save 4\n"
     L"match\n"
     L"char 'c'\n"
     L"save 3\n"
     L"match\n"},

    {L".*?((?<=_(?=[0-4][0-9]~))[0-9]+)",
     L"split 2,0\n"
     L"any\n"
     L"jmp -3\n"
     L"save 2\n"
     L"lkbehind 0,7\n"
     L"lkahead 0,4\n"
     L"cclass \"\\0-\\4\"\n"
     L"cclass \"\\0-\\9\"\n"
     L"char '~'\n"
     L"match\n"
     L"char '_'\n"
     L"match\n"
     L"cclass \"\\0-\\9\"\n"
     L"split -2,0\n"
     L"save 3\n"
     L"match\n"},

    {L"\\d\\s\\w\\D\\S\\W",
     L"cclass \":e\"\n"
     L"cclass \":i\"\n"
     L"cclass \":l\"\n"
     L"cclass \":q\"\n"
     L"cclass \":u\"\n"
     L"cclass \":x\"\n"
     L"match\n"},

    {L"[\\d\\s\\w\\D\\S\\W]",
     L"cclass \":e:i:l:q:u:x\"\n"
     L"match\n"},

    {L"a(?#comment)b",
     L"char 'a'\n"
     L"char 'b'\n"
     L"match\n"},

    {L"a(?#comment (?#(?#nest) ok))b",
     L"char 'a'\n"
     L"char 'b'\n"
     L"match\n"},

    {L"a(?#in comment parens \\(\\(\\) [(] [(] [)] must be escaped)b",
     L"char 'a'\n"
     L"char 'b'\n"
     L"match\n"},

    {L"a(?#(?<=comment)(?=out)[[:complex:]]{2,3}?expression)b",
     L"char 'a'\n"
     L"char 'b'\n"
     L"match\n"},

    {L"(?#comment (?:untouch) (save) and (counter){2,3})(a{1,3})",
     L"save 2\n"
     L"reset %0\n"
     L"rep 1,3,%0\n"
     L"split 0,2\n"
     L"char 'a'\n"
     L"jmp -4\n"
     L"save 3\n"
     L"match\n"},

    {L"(?*<|>|[^<>])",
     L"reset %0\n"
     L"jmp 4\n"
     L"split 0,2\n"
     L"char '>'\n"
     L"decjmp -3,5,%0\n"
     L"split 0,2\n"
     L"char '<'\n"
     L"incjmp -6,-6,%0\n"
     L"ncclass \"\\<\\>\"\n"
     L"jmp -8\n"
     L"match\n"},

    {L"(?*/\\*|\\*/|[^/*]|/(?!\\*)|\\*(?!/))",
     L"reset %0\n"
     L"jmp 5\n"
     L"split 0,3\n"
     L"char '*'\n"
     L"char '/'\n"
     L"decjmp -4,18,%0\n"
     L"split 0,3\n"
     L"char '/'\n"
     L"char '*'\n"
     L"incjmp -8,-8,%0\n"
     L"split 0,2\n"
     L"ncclass \"\\/\\*\"\n"
     L"jmp 10\n"
     L"split 0,5\n"
     L"char '/'\n"
     L"nlkahead 0,2\n"
     L"char '*'\n"
     L"match\n"
     L"jmp 4\n"
     L"char '*'\n"
     L"nlkahead 0,2\n"
     L"char '/'\n"
     L"match\n"
     L"jmp -22\n"
     L"match\n"},
};

int main (int argc, char* argv[])
{
    std::locale::global (std::locale (""));
    std::wcout.imbue (std::locale (""));

    int n = sizeof (spec) / sizeof (spec[0]);

    test::simple ts (n);
    for (int i = 0; i < n; i++) {
        std::wstring got (list (spec[i].input));
        ts.ok (got == spec[i].expected, esc (spec[i].input));
        if (got != spec[i].expected)
            ts.diag (got);
    }
    return ts.done_testing ();
}

