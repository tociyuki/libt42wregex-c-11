#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

typedef std::wstring::iterator derivs_t;

class vmcompiler {
public:
    bool exp (derivs_t& p, program& e);
private:
    int mgroup;
    int mreg;
    bool alt (derivs_t& p, program& e);
    bool cat (derivs_t& p, program& e);
    bool term (derivs_t& p, program& e);
    bool interval (derivs_t& p, program& e);
    bool factor (derivs_t& p, program& e);
    bool group (derivs_t& p, program& e);
    bool cclass (derivs_t& p, program& e);
    bool regchar (derivs_t& p, wchar_t& c);
    template<typename T>
    bool digits (derivs_t& p, int const base, int const len, T& x);
    int c7toi (wchar_t c);
};

bool vmcompiler::exp (derivs_t& p, program& e)
{
    mgroup = 0;
    mreg = 0;
    if (! (alt (p, e) && L'\0' == *p))
        return false;
    e.push_back (instruction (MATCH));
    return true;
}

bool vmcompiler::alt (derivs_t& p, program& e)
{
    std::vector<std::size_t> patch;
    program lhs;
    if (! cat (p, lhs))
        return false;
    while (L'|' == *p) {
        ++p;
        program rhs;
        if (! cat (p, rhs))
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

bool vmcompiler::cat (derivs_t& p, program& e)
{
    while (L'|' != *p && L')' != *p && L'\0' != *p)
        if (! term (p, e))
            return false;
    return true;
}

bool vmcompiler::term (derivs_t& p, program& e)
{
    program e1;
    if (! factor (p, e1))
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
    bool const greedy = *p != L'\x3f';
    if (! greedy)
        ++p;
    int const d = e.size ();
    int const r = mreg++;
    if (greedy)
        e.insert (e.begin (), instruction (SPLIT, 0, d + 1, 0));
    else
        e.insert (e.begin (), instruction (SPLIT, d + 1, 0, 0));
    e.insert (e.begin (), instruction (REP, n1, n2, r));
    e.insert (e.begin (), instruction (RESET, 0, 0, r));
    e.push_back (instruction (JMP, -(d + 3), 0, 0));
    return true;
}

bool vmcompiler::factor (derivs_t& p, program& e)
{
    static const std::wstring pat1 (L".^$");
    static const std::vector<operation> op1{ANY, BOL, EOL};
    static const std::wstring pat2 (L"ABbz");
    static const std::vector<operation> op2{BOS, NWORDB, WORDB, EOS};
    wchar_t c;
    std::wstring::size_type idx;
    if (L'(' == *p) {
        ++p;
        if (! group (p, e))
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
    else if (L'\\' == *p
            && ((1 <= c7toi (p[1]) && c7toi (p[1]) < 8 && c7toi (p[2]) >= 8)
                || (8 <= c7toi (p[1]) && c7toi (p[1]) < 10))) {
        int r = mreg++;
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

bool vmcompiler::group (derivs_t& p, program& e)
{
    operation op = SAVE;
    if (L'\x3f' == *p) {
        op = L':' == p[1] ? ANY
           : L'=' == p[1] ? LKAHEAD
           : L'!' == p[1] ? NLKAHEAD
           : op;
        if (SAVE == op)
            return false;
        p += 2;
    }
    int n = mgroup + 1;
    if (SAVE == op)
        e.push_back (instruction (SAVE, ++mgroup * 2, 0, 0));
    int dot = e.size ();
    if (LKAHEAD == op || NLKAHEAD == op)
        e.push_back (instruction (op, 0, 0, 0));
    if (! (alt (p, e) && L')' == *p++))
        return false;
    if (LKAHEAD == op || NLKAHEAD == op) {
        e.push_back (instruction (MATCH));
        e[dot].y = e.size () - dot - 1;
    }
    if (SAVE == op)
        e.push_back (instruction (SAVE, n * 2 + 1, 0, 0));
    return true;
}

bool vmcompiler::cclass (derivs_t& p, program& e)
{
    wchar_t c;
    operation op = L'^' == *p ? NCCLASS : CCLASS;
    if (op == NCCLASS)
        ++p;
    if (! regchar (p, c))
        return false;
    std::wstring s;
    s.push_back (L'\\');
    s.push_back (c);
    while (L']' != *p) {
        if (L'-' == *p && L']' == *(p + 1))
            c = *p++;
        else {
            if (L'-' == *p)
                s.push_back (*p++);
            if ((L'-' == *p && L']' != *(p + 1)) || ! regchar (p, c))
                return false;
        }
        s.push_back (L'\\');
        s.push_back (c);
    }
    ++p;
    e.push_back (instruction (op, s));
    return true;
}

bool vmcompiler::regchar (derivs_t& p, wchar_t& c)
{
    std::wstring::size_type idx;
    static const std::wstring pat1 (L"aeftnrv");
    static const std::wstring val1 (L"\a\x1b\f\t\n\r\v");
    if ((L'\0' <= *p && *p <= '\x1f') || '\x7f' == *p)
        return false;
    c = *p++;
    if (L'\\' != c)
        return true;
    if ((L'\0' <= *p && *p <= '\x1f') || '\x7f' == *p)
        return false;
    c = *p++;
    if ((idx = pat1.find (c)) != std::wstring::npos)
        c = val1[idx];
    else if (c7toi (c) < 8) {
        --p;
        if (! digits (p, 8, 3, c))
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
    return true;
}

template<typename T>
bool vmcompiler::digits (derivs_t& p, int const base, int const len, T& x)
{
    if (c7toi (*p) >= base)
        return false;
    x = 0;
    for (int i = 0; i < len && c7toi (*p) < base; i++)
        x = x * base + c7toi (*p++);
    return true;
}

int vmcompiler::c7toi (wchar_t ch)
{
    static const std::wstring wdigit (L"0123456789");
    static const std::wstring wupper (L"ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    static const std::wstring wlower (L"abcdefghijklmnopqrstuvwxyz");
    std::wstring::size_type i;
    if ((i = wdigit.find (ch)) != std::wstring::npos)
        return i;
    if ((i = wupper.find (ch)) != std::wstring::npos)
        return i + 10;
    if ((i = wlower.find (ch)) != std::wstring::npos)
        return i + 10;
    return 36;
}

}//namespace wpike

wregex::wregex (std::wstring s)
{
    wpike::vmcompiler comp;
    s.push_back (L'\0');
    std::wstring::iterator p = s.begin ();
    if (! comp.exp (p, e))
        throw regex_error ();
}

}//namespace t42
