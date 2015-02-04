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
    bool group (derivs_t& p, program& e, int const d);
    bool cclass (derivs_t& p, program& e) const;
    bool clschar (derivs_t& p, std::wstring& s) const;
    bool posixname (derivs_t& p, std::wstring& s) const;
    bool regchar (derivs_t& p, wchar_t& c) const;
    template<typename T>
    bool digits (derivs_t& p, int const base, int const len, T& x) const;
};

bool vmcompiler::exp (derivs_t& p, program& e, int const d)
{
    mgroup = 0;
    mreg = 0;
    if (! (alt (p, e, d) && L'\0' == *p))
        return false;
    e.push_back (instruction (MATCH));
    return true;
}

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

bool vmcompiler::interval (derivs_t& p, program& e)
{
    int n1, n2;
    derivs_t const p0 = p++;   // skip L'{'
    if (! digits (p, 10, 8, n1)) {
        p = p0;
        return true;
    }
    n2 = n1;
    if (L',' == *p) {
        ++p;
        n2 = -1;
        digits (p, 10, 8, n2);
    }
    if (L'}' != *p++) {
        p = p0;
        return true;
    }
    if (n2 != -1 && (n1 > n2 || n2 <= 0))
        return false;
    bool const greedy = *p != L'?';
    if (! greedy)
        ++p;
    int const d = e.size ();
    int const r = mreg++;
    if (n1 == n2 || greedy)
        e.insert (e.begin (), instruction (SPLIT, 0, d + 1, 0));
    else
        e.insert (e.begin (), instruction (SPLIT, d + 1, 0, 0));
    e.insert (e.begin (), instruction (REP, n1, n2, r));
    e.insert (e.begin (), instruction (RESET, 0, 0, r));
    e.push_back (instruction (JMP, -(d + 3), 0, 0));
    return true;
}

bool vmcompiler::factor (derivs_t& p, program& e, int const d)
{
    static const std::wstring pat1 (L".^$");
    static const std::vector<operation> op1{ANY, BOL, EOL};
    static const std::wstring pat2 (L"ABbz");
    static const std::vector<operation> op2{BOS, NWORDB, WORDB, EOS};
    static const std::wstring pat3 (L"dswDSW");
    static const std::wstring ccl3 (L"eilqux");
    wchar_t c;
    std::wstring::size_type idx;
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
    else if (L'\\' == *p && (idx = pat2.find (p[1])) != std::wstring::npos) {
        e.push_back (instruction (op2[idx]));
        p += 2;
    }
    else if (L'\\' == *p && (idx = pat3.find (p[1])) != std::wstring::npos) {
        std::wstring s (L":");
        s.push_back (ccl3[idx]);
        e.push_back (instruction (CCLASS, s));
        p += 2;
    }
    else if (L'\\' == *p
            && ((1 <= c7toi (p[1]) && c7toi (p[1]) < 8 && c7toi (p[2]) >= 8)
                || (8 <= c7toi (p[1]) && c7toi (p[1]) < 10))) {
        int const r = mreg++;
        e.push_back (instruction (RESET, 0, 0, r));
        e.push_back (instruction (BKREF, c7toi (p[1]), 0, r));
        p += 2;
    }
    else if (L'?' == *p || L'*' == *p || L'+' == *p)
        return false;
    else if (regchar (p, c))
        e.push_back (instruction (CHAR, std::wstring (1, c)));
    else
        return false;
    return true;
}

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

bool vmcompiler::cclass (derivs_t& p, program& e) const
{
    wchar_t c;
    operation op = L'^' == *p ? NCCLASS : CCLASS;
    if (op == NCCLASS)
        ++p;
    std::wstring s;
    if (! clschar (p, s))
        return false;
    while (L']' != *p) {
        if (L'-' == *p && L']' == *(p + 1))
            clschar (p, s);
        else {
            if (L'-' == *p)
                s.push_back (*p++);
            if ((L'-' == *p && L']' != *(p + 1)))
                return false;
            if (! clschar (p, s))
                return false;
        }
    }
    ++p;
    e.push_back (instruction (op, s));
    return true;
}

bool vmcompiler::clschar (derivs_t& p, std::wstring& s) const
{
    static const std::wstring pat (L"dswDSW");
    static const std::wstring ccl (L"eilqux");
    std::wstring::size_type i;
    wchar_t c;
    if (L'\\' == *p && (i = pat.find (p[1])) != std::wstring::npos) {
        s.push_back (L':');
        s.push_back (ccl[i]);
        p += 2;
    }
    else if (L'[' == *p && L':' == p[1]) {
        if (! posixname (p, s))
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

bool vmcompiler::posixname (derivs_t& p, std::wstring& s) const
{
    static const std::wstring lower (L"abcdefghijklmnopqrstuvwxyz");
    static const std::wstring ctname (
        L"alnum   alpha   blank   cntrl   digit   graph   lower   print   "
        L"space   upper   xdigit  word    ^alnum  ^alpha  ^blank  ^cntrl  "
        L"^digit  ^graph  ^lower  ^print  ^space  ^upper  ^xdigit ^word   ");
    static const std::vector<std::wstring> ctalias{
        L"d", L"digit", L"^d", L"^digit", L"s", L"space", L"^s", L"^space",
        L"w", L"word",  L"^w", L"^word"};
    p += 2;
    auto p0 = p;
    if (L'^' == *p)
        ++p;
    while (c7toi (*p) < 36)
        ++p;
    auto p1 = p;
    if (p1 - p0 <= 0 || L':' != *p++ || L']' != *p++)
        return false;
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

bool vmcompiler::regchar (derivs_t& p, wchar_t& c) const
{
    std::wstring::size_type idx;
    static const std::wstring pat1 (L"aftnrv");
    static const std::wstring val1 (L"\a\f\t\n\r\v");
    if (std::iswcntrl (*p))
        return false;
    c = *p++;
    if (L'\\' != c)
        return true;
    if (std::iswcntrl (*p))
        return false;
    c = *p++;
    if ((idx = pat1.find (c)) != std::wstring::npos)
        c = val1[idx];
    else if (c7toi (c) < 8) {
        --p;
        if (! digits (p, 8, 4, c))
            return false;
    }
    else if (L'x' == c) {
        if (L'{' == *p) {
            ++p;
            if (! digits (p, 16, 8, c) || L'}' != *p++)
                return false;
        }
        else if (! digits (p, 16, 2, c))
            return false;
    }
    else if (L'c' == c) {
        if (std::iswcntrl (*p))
            return false;
        c = *p++ % 32;
    }
    else if ((L'u' == c || L'U' == c) && c7toi (p[0]) < 16) {
        int const n = L'u' == c ? 4 : 8;
        derivs_t p0 = p;
        if (! digits (p, 16, n, c) || p - p0 != n)
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
