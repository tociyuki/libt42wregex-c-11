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
        if (0x20 <= c && c < 0x7e)
            t.push_back (c);
        else if (L'\t' == c)
            t += L"\\t";
        else if (0x07 == c)
            t += L"\\a";
        else if (L'\f' == c)
            t += L"\\f";
        else if (L'\r' == c)
            t += L"\\r";
        else if (L'\n' == c)
            t += L"\\n";
        else if (0x1b == c)
            t += L"\\e";
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
    std::vector<t42::wpike::vmcode> prog = re.get_prog ();
    for (auto x : prog)
        switch (x.opcode) {
        case t42::wpike::vmcode::MATCH:
            t += L"match\n";
            break;
        case t42::wpike::vmcode::SAVE:
            t += L"save ";
            t += std::to_wstring (x.addr0);
            t += L"\n";
            break;
        case t42::wpike::vmcode::CHAR:
            t += L"char '";
            t += esc (x.ch);
            t += L"'\n";
            break;
        case t42::wpike::vmcode::ANY:
            t += L"any\n";
            break;
        case t42::wpike::vmcode::CCLASS:
        case t42::wpike::vmcode::NCCLASS:
            if (t42::wpike::vmcode::CCLASS == x.opcode)
                t += L"cclass";
            else
                t += L"ncclass";
            for (auto y : *(x.span)) {
                if (t42::wpike::vmspan::RANGE == y.type) {
                    t += L" '";
                    t += esc (y.str[0]);
                    t += L"'-'";
                    t += esc (y.str[1]);
                    t += L"'";
                }
                else {
                    t += L" \"";
                    t += esc (y.str);
                    t += L"\"";
                }
            }
            t += L"\n";
            break;
        case t42::wpike::vmcode::BOL:
            t += L"bol\n";
            break;
        case t42::wpike::vmcode::EOL:
            t += L"eol\n";
            break;
        case t42::wpike::vmcode::BOS:
            t += L"bos\n";
            break;
        case t42::wpike::vmcode::EOS:
            t += L"eos\n";
            break;
        case t42::wpike::vmcode::WORDB:
            t += L"wordb\n";
            break;
        case t42::wpike::vmcode::NWORDB:
            t += L"nwordb\n";
            break;
        case t42::wpike::vmcode::JMP:
            t += L"jmp ";
            t += std::to_wstring (x.addr0);
            t += L"\n";
            break;
        case t42::wpike::vmcode::SPLIT:
            t += L"split ";
            t += std::to_wstring (x.addr0);
            t += L",";
            t += std::to_wstring (x.addr1);
            t += L"\n";
            break;
        case t42::wpike::vmcode::RESET:
            t += L"reset %r";
            t += std::to_wstring (x.reg);
            t += L"\n";
            break;
        case t42::wpike::vmcode::ISPLIT:
            t += L"isplit ";
            t += std::to_wstring (x.addr0);
            t += L",";
            t += std::to_wstring (x.addr1);
            t += L",%r";
            t += std::to_wstring (x.reg);
            t += L",$";
            t += std::to_wstring (x.n1);
            t += L",$";
            t += std::to_wstring (x.n2);
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
     L"reset %r0\n"
     L"isplit 0,2,%r0,$0,$3\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{0,3}?",
     L"reset %r0\n"
     L"isplit 2,0,%r0,$0,$3\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{1,3}",
     L"reset %r0\n"
     L"isplit 0,2,%r0,$1,$3\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{1,3}?",
     L"reset %r0\n"
     L"isplit 2,0,%r0,$1,$3\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{2}",
     L"reset %r0\n"
     L"isplit 0,2,%r0,$2,$2\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{2}?",
     L"reset %r0\n"
     L"isplit 2,0,%r0,$2,$2\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{2,4}",
     L"reset %r0\n"
     L"isplit 0,2,%r0,$2,$4\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{2,4}?",
     L"reset %r0\n"
     L"isplit 2,0,%r0,$2,$4\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{2,}",
     L"reset %r0\n"
     L"isplit 0,2,%r0,$2,$-1\n"
     L"char 'a'\n"
     L"jmp -3\n"
     L"match\n"},

    {L"a{2,}?",
     L"reset %r0\n"
     L"isplit 2,0,%r0,$2,$-1\n"
     L"char 'a'\n"
     L"jmp -3\n"
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

    {L"\\e",
     L"char '\\e'\n"
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

    {L"\\0a",
     L"char '\\x00'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\12a",
     L"char '\\n'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\012a",
     L"char '\\n'\n"
     L"char 'a'\n"
     L"match\n"},

    {L"\\0123",
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

    {L"[a]",
     L"cclass \"a\"\n"
     L"match\n"},

    {L"[abc]",
     L"cclass \"abc\"\n"
     L"match\n"},

    {L"[a-z]",
     L"cclass 'a'-'z'\n"
     L"match\n"},

    {L"[\\x30-\\x39]",
     L"cclass '0'-'9'\n"
     L"match\n"},

    {L"[a\\]\\-z]",
     L"cclass \"a]-z\"\n"
     L"match\n"},

    {L"[ab-fg]",
     L"cclass \"a\" 'b'-'f' \"g\"\n"
     L"match\n"},

    {L"[$^.?*+\\-()]",
     L"cclass \"$^.?*+-()\"\n"
     L"match\n"},

    {L"[-]",
     L"cclass \"-\"\n"
     L"match\n"},

    {L"[-a]",
     L"cclass \"-a\"\n"
     L"match\n"},

    {L"[a-]",
     L"cclass \"a-\"\n"
     L"match\n"},

    {L"[-a-]",
     L"cclass \"-a-\"\n"
     L"match\n"},

    {L"[-a-z-]",
     L"cclass \"-\" 'a'-'z' \"-\"\n"
     L"match\n"},

    {L"[[]",
     L"cclass \"[\"\n"
     L"match\n"},

    {L"[]]",
     L"cclass \"]\"\n"
     L"match\n"},

    {L"[^a]",
     L"ncclass \"a\"\n"
     L"match\n"},

    {L"[^abc]",
     L"ncclass \"abc\"\n"
     L"match\n"},

    {L"[^a-z]",
     L"ncclass 'a'-'z'\n"
     L"match\n"},

    {L"[^ab-fg]",
     L"ncclass \"a\" 'b'-'f' \"g\"\n"
     L"match\n"},

    {L"[^$^.?*+\\-()]",
     L"ncclass \"$^.?*+-()\"\n"
     L"match\n"},

    {L"[^-]",
     L"ncclass \"-\"\n"
     L"match\n"},

    {L"[^-a]",
     L"ncclass \"-a\"\n"
     L"match\n"},

    {L"[^a-]",
     L"ncclass \"a-\"\n"
     L"match\n"},

    {L"[^-a-]",
     L"ncclass \"-a-\"\n"
     L"match\n"},

    {L"[^-a-z-]",
     L"ncclass \"-\" 'a'-'z' \"-\"\n"
     L"match\n"},

    {L"[^[]",
     L"ncclass \"[\"\n"
     L"match\n"},

    {L"[^]]",
     L"ncclass \"]\"\n"
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

