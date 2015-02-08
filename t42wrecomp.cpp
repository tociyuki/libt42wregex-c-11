#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <cwctype>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

typedef std::wstring::iterator derivs_t;

bool scan (derivs_t& sp0, std::wstring pat, std::vector<derivs_t>& m)
{
    m.clear ();
    derivs_t sp = sp0;
    derivs_t ip = pat.begin ();
    while (ip < pat.end ()) {
        int lo = 0, hi = 0, n1 = 1, n2 = 1;
        wchar_t c = *ip++;
        if (L'(' == c || L')' == c)
            m.push_back (sp);
        else {
            bool dot = L'.' == c;
            if (L'%' == c) {
                c = *ip++;
                lo = L'a' == c ? 10 : lo;
                hi = L'o' == c ? 10 : L'd' == c ? 10 : L'x' == c ? 16 : (L'l' == c || L'a' == c) ? 36 : hi;
            }
            if (ip + 4 < pat.end () && L'{' == *ip && L',' == ip[2] && L'}' == ip[4])
                n1 = c7toi (ip[1]), n2 = c7toi (ip[3]), ip += 5;
            for (int i = 0; i < n2; ++i, ++sp) {
                if (L'\0' != *sp) {
                    int x = c7toi (*sp);
                    if (dot || (hi == 0 && c == *sp) || (lo <= x && x < hi))
                        continue;
                }
                if (i < n1)
                    return false;
                break;
            }
        }
    }
    sp0 = sp;
    return true;
}

bool scan (derivs_t& p, std::wstring pat)
{
    std::vector<derivs_t> _;
    return scan (p, pat, _);
}

bool scan_not (derivs_t& p, std::wstring pat, std::wstring la)
{
    derivs_t q = p;
    if (scan (q, pat) && ! scan (q, la)) {
        p = q;
        return true;
    }
    return false;
}

template<typename T>
bool scan_digits (derivs_t& p, int const base, int const len, T& x)
{
    if (c7toi (*p) >= base)
        return false;
    x = 0;
    for (int i = 0; i < len && c7toi (*p) < base; ++i, ++p)
        x = x * base + c7toi (*p);
    return true;
}

struct vmlex {
    vmlex () {}
    virtual ~vmlex () {}
    virtual bool endstring (derivs_t& p) { return L'\0' == *p; }
    virtual bool alt (derivs_t& p) { return scan (p, L"|"); }
    virtual bool rep01 (derivs_t& p) { return scan (p, L"?"); }
    virtual bool rep0 (derivs_t& p) { return scan (p, L"*"); }
    virtual bool rep1 (derivs_t& p) { return scan (p, L"+"); }
    virtual bool ngreedy (derivs_t& p) { return scan (p, L"?"); }
    virtual bool any (derivs_t& p) { return scan (p, L"%."); }
    virtual bool bol (derivs_t& p) { return scan (p, L"^"); }
    virtual bool eol (derivs_t& p) { return scan (p, L"$"); }
    virtual bool bos (derivs_t& p) { return scan (p, L"\\A"); }
    virtual bool eos (derivs_t& p) { return scan (p, L"\\z"); }
    virtual bool wordb (derivs_t& p) { return scan (p, L"\\b"); }
    virtual bool nwordb (derivs_t& p) { return scan (p, L"\\B"); }
    virtual bool first_group (derivs_t& p) { return L'(' == *p; }
    virtual bool group (derivs_t& p) { return scan_not (p, L"%(", L"?"); }
    virtual bool lparen (derivs_t& p) { return scan (p, L"%(?:"); }
    virtual bool lkahead (derivs_t& p) { return scan (p, L"%(?="); }
    virtual bool nlkahead (derivs_t& p) { return scan (p, L"%(?!"); }
    virtual bool lkbehind (derivs_t& p) { return scan (p, L"%(?<="); }
    virtual bool nlkbehind (derivs_t& p) { return scan (p, L"%(?<!"); }
    virtual bool gcomment (derivs_t& p) { return scan (p, L"%(?#"); }
    virtual bool rparen (derivs_t& p) { return scan (p, L"%)"); }
    virtual bool cclass (derivs_t& p) { return scan (p, L"["); }
    virtual bool ncclass (derivs_t& p) { return scan (p, L"^"); }
    virtual bool rcclass (derivs_t& p) { return scan (p, L"]"); }
    virtual bool range (derivs_t& p) { return scan_not (p, L"-", L"]"); }

    virtual bool repnn (derivs_t& p, int& n1, int& n2)
    {
        std::vector<derivs_t> m;
        if (scan (p, L"%{(%d{1,8})%}", m)) {
            scan_digits (m[0], 10, 8, n1);
            n2 = n1;
            return true;
        }
        else if (scan (p, L"%{(%d{1,8}),(%d{0,8})%}", m)) {
            n2 = -1;
            scan_digits (m[0], 10, 8, n1);
            scan_digits (m[2], 10, 8, n2);
            return true;
        }
        return false;
    }

    virtual bool bsname (derivs_t& p, std::wstring& t)
    {
        static const std::wstring name (L"dswDSW");
        if (L'\\' == *p && name.find (p[1]) != std::wstring::npos) {
            t.assign (1, p[1]);
            p += 2;
            return true;
        }
        return false;
    }

    virtual bool bkref (derivs_t& p, int& n)
    {
        if (L'\\' != *p)
            return false;
        n = c7toi (p[1]);
        if ((1 <= n && n <= 7 && c7toi (p[2]) >= 8) || (8 <= n && n < 10)) {
            p += 2;
            return true;
        }
        return false;
    }

    virtual bool regchar (derivs_t& p, wchar_t& c)
    {
        static const std::wstring ctrlname (L"aftnrv");
        static const std::wstring ctrlchar (L"\a\f\t\n\r\v");
        std::wstring::size_type i;
        std::vector<derivs_t> m;
        if (scan (p, L"\\c.")) {
            if (std::iswcntrl (p[-1]))
                return false;
            c = p[-1] % 32;
        }
        else if (scan (p, L"\\(%o{1,4})", m))
            scan_digits (m[0], 8, 4, c);
        else if (scan (p, L"\\x(%x{1,2})", m))
            scan_digits (m[0], 16, 2, c);
        else if (scan (p, L"\\x%{(%x{1,8})%}", m))
            scan_digits (m[0], 16, 8, c);
        else if (scan (p, L"\\u(%x{4,4})", m))
            scan_digits (m[0], 16, 4, c);
        else if (scan (p, L"\\U(%x{8,8})", m))
            scan_digits (m[0], 16, 8, c);
        else {
            c = *p++;
            if (std::iswcntrl (c))
                return false;
            if (L'\\' == c) {
                c = *p++;
                if (std::iswcntrl (c))
                    return false;
                if ((i = ctrlname.find (c)) != std::wstring::npos)
                    c = ctrlchar[i];
            }
        }
        return true;
    }

    virtual bool posixname (derivs_t& p, std::wstring& t)
    {
        std::vector<derivs_t> m;
        if (scan (p, L"[:(^{0,1}%a{1,z}):]", m)) {
            t.assign (m[0], m[1]);
            return true;
        }
       return false;
    }

    virtual bool first_term (derivs_t& p)
    {
        return L'|' != *p && L')' != *p && L'\0' != *p;
    }

    virtual bool first_factor (derivs_t& p)
    {
        std::wstring _;
        return L'?' != *p && L'*' != *p && L'+' != *p && ! posixname (p, _);
    }
};

struct compenv {
    bool behind;
};

class vmcompiler {
public:
    vmcompiler (std::shared_ptr<vmlex> const& a) : lex (a) {}
    bool exp (derivs_t& p, program& e);
private:
    std::shared_ptr<vmlex> lex;
    int mgroup;
    int mreg;
    bool alt (derivs_t& p, compenv& a, program& e);
    bool cat (derivs_t& p, compenv& a, program& e);
    bool term (derivs_t& p, compenv& a, program& e);
    bool factor (derivs_t& p, compenv& a, program& e);
    bool group (derivs_t& p, compenv& a, program& e);
    bool gcomment (derivs_t& p);
    bool cclass (derivs_t& p, compenv& a, program& e);
    bool clschar (derivs_t& p, compenv& a, std::wstring& span);
    bool encode_posixname (std::wstring name, std::wstring& s);
};

// exp <- alt ENDSTR
//
//      e
//      MATCH
bool vmcompiler::exp (derivs_t& p, program& e)
{
    compenv a;
    a.behind = false;
    mgroup = 0;
    mreg = 0;
    if (! (alt (p, a, e) && lex->endstring (p)))
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
bool vmcompiler::alt (derivs_t& p, compenv& a, program& e)
{
    std::vector<std::size_t> patch;
    program lhs;
    if (! cat (p, a, lhs))
        return false;
    while (lex->alt (p)) {
        program rhs;
        if (! cat (p, a, rhs))
            return false;
        if (lhs.empty () && rhs.empty ())
            continue;
        e.push_back (instruction (SPLIT, 0, lhs.size () + 1, 0));
        e.insert (e.end (), lhs.begin (), lhs.end ());
        patch.push_back (e.size ());
        e.push_back (instruction (JMP, 0, 0, 0));
        lhs = std::move (rhs);
    }
    e.insert (e.end (), lhs.begin (), lhs.end ());
    std::size_t const dot = e.size ();
    for (auto i : patch)
        e[i].x = dot - i - 1;
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
bool vmcompiler::cat (derivs_t& p, compenv& a, program& e)
{
    while (lex->first_term (p)) {
        program e1;
        if (! term (p, a, e1))
            return false;
        if (a.behind)
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
//
// e{m,n}                      e{m,n}?
//      RESET %r                    RESET %r
//   L1 REP   m,n,%r             L1 REP   m,n,%r
//      SPLIT L2,L3                 SPLIT L3,L2
//   L2 e                        L2 e
//      JMP   L1                    JMP   L1
//   L3                          L3
bool vmcompiler::term (derivs_t& p, compenv& a, program& e)
{
    program e1;
    int k1 = 1, k2 = 1;
    bool ngreedy = false;
    if (! factor (p, a, e1))
        return false;
    if (lex->rep01 (p)) {
        k1 = 0, k2 = 1;
        ngreedy = lex->ngreedy (p);
    }
    else if (lex->rep0 (p)) {
        k1 = 0, k2 = -1;
        ngreedy = lex->ngreedy (p);
    }
    else if (lex->rep1 (p)) {
        k1 = 1, k2 = -1;
        ngreedy = lex->ngreedy (p);
    }
    else if (lex->repnn (p, k1, k2)) {
        ngreedy = lex->ngreedy (p);
    }
    int n1 = e1.size ();
    int x = k1 == 1 && k2 == -1 ? -(n1 + 1) : 0;
    int y = k1 == 0 && k2 == 1 ? n1 : k1 == 1 && k2 == -1 ? 0 : n1 + 1;
    if (k1 != k2 && ngreedy)
        std::swap (x, y);
    if (k1 == 1 && k2 == 1)
        e.insert (e.end (), e1.begin (), e1.end ());
    else if (k1 == 1 && k2 == -1) {
        e.insert (e.end (), e1.begin (), e1.end ());
        e.push_back (instruction (SPLIT, x, y, 0));
    }
    else {
        int z = k1 == 0 && k2 == -1 ? -(n1 + 2) : -(n1 + 3);
        if (! (k1 == 0 && (k2 == 1 || k2 == -1))) {
            int const r = mreg++;
            e.push_back (instruction (RESET, 0, 0, r));
            e.push_back (instruction (REP, k1, k2, r));
        }
        e.push_back (instruction (SPLIT, x, y, 0));
        e.insert (e.end (), e1.begin (), e1.end ());
        if (! (k1 == 0 && k2 == 1))
            e.push_back (instruction (JMP, z, 0, 0));
    }
    return true;
}

// factor <- group / cclass / [.] / assertions / regchar
// assertions <- '\\' ([ABbzdswDSW] / [1-7] ![0-7] / [89])
//
// .             ^             $            a
//     ANY           BOL            EOL          CHAR  "a"
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
bool vmcompiler::factor (derivs_t& p, compenv& a, program& e)
{
    std::wstring name;
    int n;
    wchar_t c;
    if (! lex->first_factor (p))
        return false;
    else if (lex->first_group (p)) {
        if (! group (p, a, e))
            return false;
    }
    else if (lex->cclass (p)) {
        if (! cclass (p, a, e))
            return false;
    }
    else if (lex->any (p))
        e.push_back (instruction (ANY));
    else if (lex->bol (p))
        e.push_back (instruction (BOL));
    else if (lex->eol (p))
        e.push_back (instruction (EOL));
    else if (lex->bos (p))
        e.push_back (instruction (BOS));
    else if (lex->eos (p))
        e.push_back (instruction (EOS));
    else if (lex->wordb (p))
        e.push_back (instruction (WORDB));
    else if (lex->nwordb (p))
        e.push_back (instruction (NWORDB));
    else if (lex->bsname (p, name)) {
        std::wstring s;
        encode_posixname (name, s);
        e.push_back (instruction (CCLASS, s));
    }
    else if (lex->bkref (p, n)) {
        int const r = mreg++;
        e.push_back (instruction (RESET, 0, 0, r));
        e.push_back (instruction (BKREF, n, 0, r));
    }
    else {
        if (! lex->regchar (p, c))
            return false;
        e.push_back (instruction (CHAR, std::wstring (1, c)));
    }
    return true;
}

// group <- '(' ('?:' / '?=' / '?!' / '?<=' / '?<!' / '?#')? alt ')'
//
// (e)                    (?:e)                 e1(?#e)e2
//       SAVE  2*n              e                   e1
//       e                                          e2
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
bool vmcompiler::group (derivs_t& p, compenv& a, program& e)
{
    if (lex->gcomment (p))
        return gcomment (p);
    operation op = lex->group (p) ? SAVE
                 : lex->lparen (p) ? MATCH
                 : lex->lkahead (p) ? LKAHEAD
                 : lex->nlkahead (p) ? NLKAHEAD
                 : lex->lkbehind (p) ? LKBEHIND
                 : lex->nlkbehind (p) ? NLKBEHIND
                 : ANY;
    if (ANY == op)
        return false;
    int n1 = (mgroup + 1) * 2, n2 = (mgroup + 1) * 2 + 1;
    if (SAVE == op) {
        ++mgroup;
        if (a.behind)
            std::swap (n1, n2);
        e.push_back (instruction (SAVE, n1, 0, 0));
    }
    int const dot = e.size ();
    if (LKAHEAD == op || NLKAHEAD == op || LKBEHIND == op || NLKBEHIND == op)
        e.push_back (instruction (op, 0, 0, 0));
    compenv a1 = a;
    if (LKAHEAD == op || NLKAHEAD == op)
        a1.behind = false;
    else if (LKBEHIND == op || NLKBEHIND == op)
        a1.behind = true;
    if (! (alt (p, a1, e) && lex->rparen (p)))
        return false;
    if (LKAHEAD == op || NLKAHEAD == op || LKBEHIND == op || NLKBEHIND == op) {
        e.push_back (instruction (MATCH, 0, 0, 0));
        e[dot].y = e.size () - dot - 1;
    }
    if (SAVE == op)
        e.push_back (instruction (SAVE, n2, 0, 0));
    return true;
}

bool vmcompiler::gcomment (derivs_t& p)
{
    std::wstring s;
    wchar_t c;
    while (! lex->rparen (p)) {
        if (lex->first_group (p)) {
            lex->regchar (p, c);
            if (! gcomment (p))
                return false;
        }
        else if (lex->cclass (p)) {
            lex->ncclass (p);
            do {
                if (! lex->posixname (p, s) && ! lex->regchar (p, c))
                    return false;
            } while (! lex->rcclass (p));
        }
        else if (! lex->regchar (p, c))
            return false;
    }
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
bool vmcompiler::cclass (derivs_t& p, compenv& a, program& e)
{
    std::wstring s;
    operation op = lex->ncclass (p) ? NCCLASS : CCLASS;
    if (! clschar (p, a, s))
        return false;
    while (! lex->rcclass (p)) {
        if (lex->range (p))
            s.push_back (L'-');
        if (lex->range (p))
            return false;
        if (! clschar (p, a, s))
            return false;
    }
    e.push_back (instruction (op, s));
    return true;
}

// clschar <- '\\' [dswDWS] / posixname / regchar
bool vmcompiler::clschar (derivs_t& p, compenv& a, std::wstring& s)
{
    std::wstring name;
    if (lex->posixname (p, name)) {
        if (! encode_posixname (name, s))
            return false;
    }
    else if (lex->bsname (p, name))
        encode_posixname (name, s);
    else {
        wchar_t c;
        if (! lex->regchar (p, c))
            return false;
        s.push_back (L'\\');
        s.push_back (c);
    }
    return true;
}

// posixnames are encoded an alphabet character
// in the span wstring of the CCLASS and the NCCLASS instruction
//
// [:alnum:] -> ":a",  [:alpha:] -> ":b",  [:blank:] -> ":c", ..
//
// additional names are [:word:] for \w, [:^word:] for \W  
bool vmcompiler::encode_posixname (std::wstring name, std::wstring& s)
{
    static const std::wstring lower (L"abcdefghijklmnopqrstuvwxyz");
    static const std::wstring pxname (
        L"alnum   alpha   blank   cntrl   digit   graph   lower   print   "
        L"space   upper   xdigit  word    ^alnum  ^alpha  ^blank  ^cntrl  "
        L"^digit  ^graph  ^lower  ^print  ^space  ^upper  ^xdigit ^word   ");
    static const std::vector<std::wstring> pxalias{
        L"d", L"digit", L"^d", L"^digit", L"D", L"^digit",
        L"s", L"space", L"^s", L"^space", L"S", L"^space",
        L"w", L"word",  L"^w", L"^word",  L"W", L"^word"};
    for (int j = 0; j < pxalias.size (); j += 2)
        if (name == pxalias[j]) {
            name = pxalias[j + 1];
            break;
        }
    std::wstring::size_type i = pxname.find (name);
    if (i == std::wstring::npos)
        return false;
    s.push_back (L':');
    s.push_back (lower[i / 8]);
    return true;
}

}//namespace wpike

wregex::wregex (std::wstring s)
{
    auto lex = std::make_shared<wpike::vmlex> ();
    wpike::vmcompiler comp (lex);
    flag = 0;
    s.push_back (L'\0');
    std::wstring::iterator p = s.begin ();
    if (! comp.exp (p, e))
        throw regex_error ();
}

wregex::wregex (std::wstring s, flag_type f)
{
    auto lex = std::make_shared<wpike::vmlex> ();
    wpike::vmcompiler comp (lex);
    flag = f;
    s.push_back (L'\0');
    std::wstring::iterator p = s.begin ();
    if (! comp.exp (p, e))
        throw regex_error ();
}

}//namespace t42
