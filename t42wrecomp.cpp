#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <cwctype>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

typedef std::wstring::iterator derivs_t;

class vmcompiler {
public:
    bool exp (derivs_t& p, program& e, int const d);
private:
    int mgroup;
    int mreg;
    bool alt (derivs_t& p, program& e, int const d);
    bool cat (derivs_t& p, program& , int const d);
    bool term (derivs_t& p, program& e, int const d);
    bool interval (derivs_t& p, program& e);
    bool factor (derivs_t& p, program& e, int const d);
    bool assertions (derivs_t& p, program& e);
    bool group (derivs_t& p, program& e, int const d);
    bool cclass (derivs_t& p, program& e) const;
    bool clschar (derivs_t& p, std::wstring& s) const;
    bool posixname (derivs_t& p) const;
    bool set_posixname (derivs_t p0, derivs_t p1, std::wstring& s) const;
    bool regchar (derivs_t& p, wchar_t& c) const;
    template<typename T>
    bool digits (derivs_t& p, int const base, int const len, T& x) const;
};

// exp <- alt ENDSTR
//
//      e
//      MATCH
bool vmcompiler::exp (derivs_t& p, program& e, int const d)
{
    mgroup = 0;
    mreg = 0;
    if (! (alt (p, e, d) && L'\0' == *p))
        return false;
    e.push_back (instruction (MATCH));
    return true;
}

// alt <- cat ('|' cat)*
//
// instructions SPLIT and JMP have relative address displacements
// from the next instruction.
//
//      SPLIT   L1,L2
//   L1 e1
//      JMP     L6
//   L2 SPLIT   L3,L4
//   L3 e2
//      JMP     L6
//   L4 SPLIT   L5,L6
//   L5 e3
//   L6
bool vmcompiler::alt (derivs_t& p, program& e, int const d)
{
    std::vector<std::size_t> patch;
    program lhs;
    if (! cat (p, lhs, d))
        return false;
    while (L'|' == *p) {
        ++p;
        program rhs;
        if (! cat (p, rhs, d))
            return false;
        if (lhs.size () == 0 && rhs.size () == 0)
            continue;
        e.push_back (instruction (SPLIT, 0, lhs.size () + 1, 0));
        e.insert (e.end (), lhs.begin (), lhs.end ());
        patch.push_back (e.size ());
        e.push_back (instruction (JMP, 0, 0, 0));
        lhs = std::move (rhs);
    }
    e.insert (e.end (), lhs.begin (), lhs.end ());
    std::size_t const dol = e.size ();
    for (auto i : patch)
        e[i].x = dol - i - 1;
    return true;
}

// cat <- term*
//
//  e1 e2           where d > 0 outside LOOKBEHIND or inside LOOKAHEAD
//      e1
//      e2
//
//  (?<= e1 e2)     where d < 0 inside LOOKBEHIND
//      e2
//      e1
bool vmcompiler::cat (derivs_t& p, program& e, int const d)
{
    while (L'|' != *p && L')' != *p && L'\0' != *p) {
        program e1;
        if (! term (p, e1, d))
            return false;
        if (d < 0)
            e.insert (e.begin (), e1.begin (), e1.end ());
        else
            e.insert (e.end (), e1.begin (), e1.end ());
    }
    return true;
}

// term <- factor ([?*+]'?'? / interval)?
//
// e?                         e??
//      SPLIT L1,L2                 SPLIT L2,L1
//   L1 e                        L1 e
//   L2                          L2
//
// e*                         e*?
//   L1 SPLIT L2,L3              L1 SPLIT L3,L2
//   L2 e                        L2 e
//      JMP   L1                    JMP   L1
//   L3                          L3
//
// e+                         e+?
//   L1 e                        L1 e
//      SPLIT L1,L2                 SPLIT L2,L1
//   L2                          L2
bool vmcompiler::term (derivs_t& p, program& e, int const d)
{
    program e1;
    if (! factor (p, e1, d))
        return false;
    if (L'?' == *p || L'*' == *p || L'+' == *p) {
        wchar_t const repetition = *p++;
        bool const greedy = *p != L'?';
        int const n1 = static_cast<int> (e1.size ());
        int x = L'+' == repetition ? -(n1 + 1) : 0;
        int y = L'*' == repetition ? n1 + 1 : L'?' == repetition ? n1 : 0;
        if (! greedy) {
            ++p;
            std::swap (x, y);
        }
        if (L'+' == repetition)
            e1.push_back (instruction (SPLIT, x, y, 0));
        else {
            e1.insert (e1.begin (), instruction (SPLIT, x, y, 0));
            if (L'*' == repetition)
                e1.push_back (instruction (JMP, -(n1 + 2), 0, 0));
        }
    }
    else if (L'{' == *p && ! interval (p, e1))
        return false;
    e.insert (e.end (), e1.begin (), e1.end ());
    return true;
}

// interval <- '{' [0-9]+ (',' [0-9]*)? '}' '?'?
//
// e{m,n}                      e{m,n}?
//      RESET %r                    RESET %r
//   L1 REP   m,n,%r             L1 REP   m,n,%r
//      SPLIT L2,L3                 SPLIT L3,L2
//   L2 e                        L2 e
//      JMP   L1                    JMP   L1
//   L3                          L3
bool vmcompiler::interval (derivs_t& p, program& e)
{
    int n1, n2;
    derivs_t q = p + 1;     // skip L'{'
    if (! digits (q, 10, 8, n1))
        return true;        // backtrack for L"q{a}"
    n2 = n1;
    if (L',' == *q) {
        ++q;
        n2 = -1;
        digits (q, 10, 8, n2);
    }
    if (L'}' != *q++)
        return true;
    bool const greedy = *q != L'?';
    if (! greedy)
        ++q;
    if (n2 != -1 && (n1 > n2 || n2 <= 0))
        return false;
    int const d = e.size ();
    int const r = mreg++;
    if (n1 == n2 || greedy)
        e.insert (e.begin (), instruction (SPLIT, 0, d + 1, 0));
    else
        e.insert (e.begin (), instruction (SPLIT, d + 1, 0, 0));
    e.insert (e.begin (), instruction (REP, n1, n2, r));
    e.insert (e.begin (), instruction (RESET, 0, 0, r));
    e.push_back (instruction (JMP, -(d + 3), 0, 0));
    p = q;
    return true;
}

// factor <- group / cclass / [.^$] / assertions / regchar
//
// .             ^             $            a
//     ANY           BOL            EOL          CHAR  "a"
bool vmcompiler::factor (derivs_t& p, program& e, int const d)
{
    static const std::wstring pat1 (L".^$");
    static const std::vector<operation> op1{ANY, BOL, EOL};
    wchar_t c;
    std::wstring::size_type idx;
    if (L'?' == *p || L'*' == *p || L'+' == *p || posixname (p))
        return false;
    if (L'(' == *p) {
        ++p;
        if (! group (p, e, d))
            return false;
    }
    else if (L'[' == *p) {
        ++p;
        if (! cclass (p, e))
            return false;
    }
    else if ((idx = pat1.find (*p)) != std::wstring::npos) {
        e.push_back (instruction (op1[idx]));
        ++p;
    }
    else if (L'\\' == *p && assertions (p, e))
        ;
    else if (regchar (p, c))
        e.push_back (instruction (CHAR, std::wstring (1, c)));
    else
        return false;
    return true;
}

// assertions <- '\\' ([ABbzdswDSW] / [1-7] ![0-7] / [89])
//
// \A                 \B                  \b                   \z
//     BOS                 WORDB                NWRODB                EOS
//
// \d                 \s                  \w
//     CCLASS ":e"         CCLASS ":i"          CCLASS ":l"
//
// \D                 \S                  \W
//     CCLASS ":q"         CCLASS ":u"          CCLASS ":x"
//
// \1
//     BKREF  1
bool vmcompiler::assertions (derivs_t& p, program& e)
{
    static const std::wstring chassertion (L"ABbz");
    static const std::vector<operation> opassertion{BOS, NWORDB, WORDB, EOS};
    static const std::wstring chclass (L"dswDSW");
    static const std::wstring ccl (L"eilqux");
    std::wstring::size_type idx;
    if ((idx = chassertion.find (p[1])) != std::wstring::npos) {
        e.push_back (instruction (opassertion[idx]));
        p += 2;
    }
    else if ((idx = chclass.find (p[1])) != std::wstring::npos) {
        std::wstring s (L":");
        s.push_back (ccl[idx]);
        e.push_back (instruction (CCLASS, s));
        p += 2;
    }
    else if ((1 <= c7toi (p[1]) && c7toi (p[1]) < 8 && c7toi (p[2]) >= 8)
            || (8 <= c7toi (p[1]) && c7toi (p[1]) < 10)) {
        int const r = mreg++;
        e.push_back (instruction (RESET, 0, 0, r));
        e.push_back (instruction (BKREF, c7toi (p[1]), 0, r));
        p += 2;
    }
    else
        return false;
    return true;
}

// group <- '(' ('?:' / '?=' / '?!' / '?<=' / '?<!')? alt ')'
//
// (e)                    (?:e)
//       SAVE  2*n              e
//       e
//       SAVE  2*n+1
//
// (?=e1 e2)              (?!e1 e2)
//       LKAHEAD L1,L2          NLKAHEAD L1,L2
//    L1 e1                  L1 e1
//       e2                     e2
//       MATCH                  MATCH
//    L2                     L2
//
// (?<=e1 e2)             (?<!e1 e2)
//       LKBEHIND L1,L2         NLKBEHIND L1,L2
//    L1 e2                  L1 e2
//       e1                     e1
//       MATCH                  MATCH
//    L2                     L2
bool vmcompiler::group (derivs_t& p, program& e, int const d)
{
    operation op = SAVE;
    if (L'?' == *p) {
        op = L':' == p[1] ? ANY
           : L'=' == p[1] ? LKAHEAD
           : L'!' == p[1] ? NLKAHEAD
           : L'<' == p[1] && L'=' == p[2] ? LKBEHIND
           : L'<' == p[1] && L'!' == p[2] ? NLKBEHIND
           : op;
        if (SAVE == op)
            return false;
        p += L'<' == p[1] ? 3 : 2;
    }
    int n1 = (mgroup + 1) * 2;
    int n2 = (mgroup + 1) * 2 + 1;
    if (SAVE == op) {
        ++mgroup;
        if (d < 0)
            std::swap (n1, n2);
        e.push_back (instruction (SAVE, n1, 0, 0));
    }
    int const dot = e.size ();
    if (LKAHEAD == op || NLKAHEAD == op || LKBEHIND == op || NLKBEHIND == op)
        e.push_back (instruction (op, 0, 0, 0));
    int const d1 = (LKAHEAD == op  || NLKAHEAD == op)  ? +1
                  : (LKBEHIND == op || NLKBEHIND == op) ? -1
                  : d;
    if (! (alt (p, e, d1) && L')' == *p++))
        return false;
    if (LKAHEAD == op || NLKAHEAD == op || LKBEHIND == op || NLKBEHIND == op) {
        e.push_back (instruction (MATCH, 0, 0, 0));
        e[dot].y = e.size () - dot - 1;
    }
    if (SAVE == op)
        e.push_back (instruction (SAVE, n2, 0, 0));
    return true;
}

// cclass <- '[' '^'? clschar ('-'? clschar)* '-'? ']'
//
// in the span string, charcters are quoted by a backslash.
// posixname and perl backslash name are encoded by a colon.
// see heritage BSD ex editor's regex.c by Bill Joy.
//
// [a-z\d[:blank:]]               [^a-z\d[:blank:]]
//      CCLASS  "\\a-\\z:e:c"           NCCLASS  "\\a-\\z:e:c"
bool vmcompiler::cclass (derivs_t& p, program& e) const
{
    wchar_t c;
    operation op = L'^' == *p ? NCCLASS : CCLASS;
    if (op == NCCLASS)
        ++p;
    std::wstring s;
    if (! clschar (p, s))   // []a] or [-a] trick
        return false;
    while (L']' != *p) {
        if (L'-' == *p && L']' != *(p + 1)) // [a-] trick
            s.push_back (*p++);
        if (L'-' == *p && L']' != *(p + 1)) // [%--] trick
            return false;
        if (! clschar (p, s))
            return false;
    }
    ++p;
    e.push_back (instruction (op, s));
    return true;
}

// clschar <- '\\' [dswDWS] / posixname / regchar
bool vmcompiler::clschar (derivs_t& p, std::wstring& s) const
{
    static const std::wstring pat (L"dswDSW");
    static const std::wstring ccl (L"eilqux");  // see posixname
    std::wstring::size_type i;
    wchar_t c;
    if (L'\\' == *p && (i = pat.find (p[1])) != std::wstring::npos) {
        s.push_back (L':');
        s.push_back (ccl[i]);
        p += 2;
    }
    else if (L'[' == *p && L':' == p[1]) {  // [:posixname:]
        derivs_t p0 = p + 2;
        if (! (posixname (p) && set_posixname (p0, p - 2, s)))
            return false;
    }
    else if (regchar (p, c)) {
        s.push_back (L'\\');
        s.push_back (c);
    }
    else
        return false;
    return true;
}

// posixname <- '[:' '^'? [A-Za-z0-9]+ ':]'
bool vmcompiler::posixname (derivs_t& p) const
{
    if (! (L'[' == *p && L':' == p[1]))
        return false;
    derivs_t q = p + 2;
    if (L'^' == *q)
        ++q;
    derivs_t q0 = q;
    while (c7toi (*q) < 36)
        ++q;
    if (! (q - q0 > 0 && L':' == *q++ && L']' == *q++))
        return false;
    p = q;
    return true;
}

// posixnames are encoded an alphabet character
// in the span wstring of the CCLASS and the NCCLASS instruction
//
// [:alnum:] -> ":a",  [:alpha:] -> ":b",  [:blank:] -> ":c", ..
//
// additional names are [:word:] for \w, [:^word:] for \W  
bool vmcompiler::set_posixname (derivs_t p0, derivs_t p1, std::wstring& s) const
{
    static const std::wstring lower (L"abcdefghijklmnopqrstuvwxyz");
    static const std::wstring ctname (
        L"alnum   alpha   blank   cntrl   digit   graph   lower   print   "
        L"space   upper   xdigit  word    ^alnum  ^alpha  ^blank  ^cntrl  "
        L"^digit  ^graph  ^lower  ^print  ^space  ^upper  ^xdigit ^word   ");
    static const std::vector<std::wstring> ctalias{
        L"d", L"digit", L"^d", L"^digit", L"s", L"space", L"^s", L"^space",
        L"w", L"word",  L"^w", L"^word"};
    std::wstring name (p0, p1);
    for (int j = 0; j < ctalias.size (); j += 2)
        if (name == ctalias[j]) {
            name = ctalias[j + 1];
            break;
        }
    std::wstring::size_type i = ctname.find (name);
    if (i == std::wstring::npos)
        return false;
    s.push_back (L':');
    s.push_back (lower[i / 8]);
    return true;
}

// regchar <- '\\' [aftnrv] / '\\c' . / '\\x{' hex+ '}' / '\\x' hex hex?
//          / '\\u' hex hex hex hex / '\\U' hex hex hex hex hex hex hex hex
//          / '\\' [0-7] ([0-7] ([0-7] [0-7]?)?
//          / '\\' . / .
// hex     <- [0-9A-Fa-f]
bool vmcompiler::regchar (derivs_t& p, wchar_t& c) const
{
    std::wstring::size_type idx;
    static const std::wstring ctrlname (L"aftnrv");
    static const std::wstring ctrlchar (L"\a\f\t\n\r\v");
    if (std::iswcntrl (*p))
        return false;
    c = *p++;
    if (L'\\' != c)
        return true;
    if (std::iswcntrl (*p))
        return false;
    c = *p++;
    if ((idx = ctrlname.find (c)) != std::wstring::npos)
        c = ctrlchar[idx];
    else if (L'c' == c) {
        if (std::iswcntrl (*p))
            return false;
        c = *p++ % 32;
    }
    else if (c7toi (c) < 8) {
        --p;
        digits (p, 8, 4, c);
    }
    else if (L'x' == c && L'{' == *p) {
        ++p;
        if (! (digits (p, 16, 8, c) && L'}' == *p++))
            return false;
    }
    else if (L'x' == c || L'u' == c || L'U' == c) {
        int const n = L'x' == c ? 2 : L'u' == c ? 4 : 8;
        derivs_t p0 = p;
        if (! (digits (p, 16, n, c) && (n < 4 || p - p0 == n)))
            return false;
    }
    return true;
}

template<typename T>
bool vmcompiler::digits (derivs_t& p, int const base, int const len, T& x) const
{
    if (c7toi (*p) >= base)
        return false;
    x = 0;
    for (int i = 0; i < len && c7toi (*p) < base; i++)
        x = x * base + c7toi (*p++);
    return true;
}

}//namespace wpike

wregex::wregex (std::wstring s)
{
    wpike::vmcompiler comp;
    flag = 0;
    s.push_back (L'\0');
    std::wstring::iterator p = s.begin ();
    if (! comp.exp (p, e, +1))
        throw regex_error ();
}

wregex::wregex (std::wstring s, flag_type f)
{
    wpike::vmcompiler comp;
    flag = f;
    s.push_back (L'\0');
    std::wstring::iterator p = s.begin ();
    if (! comp.exp (p, e, +1))
        throw regex_error ();
}

}//namespace t42
