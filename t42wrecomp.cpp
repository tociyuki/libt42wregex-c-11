#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

class vmcompiler {
public:
    bool compile (std::wstring str, std::vector<vmcode>& prog);
private:
    std::wstring mstr;
    int mpos;
    int mgroup;
    int mreg;
    bool compile_exp (std::vector<vmcode>& prog);
    bool compile_cat (std::vector<vmcode>& prog);
    bool compile_term (std::vector<vmcode>& prog);
    bool compile_interval (std::vector<vmcode>& lhs);
    bool compile_factor (std::vector<vmcode>& prog);
    bool compile_cclass (std::vector<vmcode>& prog);
    bool compile_char (wchar_t& ch);
    template<typename T>
    bool digits (T& value, int const base, int const len);
    int c7toi (wchar_t ch);
};

bool vmcompiler::compile (std::wstring str, std::vector<vmcode>& prog)
{
    mstr.assign (std::move (str));
    mstr.push_back ('\0');
    mpos = 0;
    mgroup = 0;
    mreg = 0;
    if (! compile_exp (prog))
        return false;
    prog.push_back (vmcode (vmcode::MATCH));
    return true;
}

bool vmcompiler::compile_exp (std::vector<vmcode>& prog)
{
    std::vector<std::size_t> patch;
    std::vector<vmcode> lhs;
    if (! compile_cat (lhs))
        return false;
    while (L'|' == mstr[mpos]) {
        ++mpos;
        std::vector<vmcode> rhs;
        if (! compile_cat (rhs))
            return false;
        if (lhs.size () == 0 && rhs.size () == 0)
            continue;
        prog.push_back (vmcode (vmcode::SPLIT, 0, lhs.size () + 1));
        prog.insert (prog.end (), lhs.begin (), lhs.end ());
        patch.push_back (prog.size ());
        prog.push_back (vmcode (vmcode::JMP, 0, 0));
        lhs = std::move (rhs);
    }
    prog.insert (prog.end (), lhs.begin (), lhs.end ());
    std::size_t const ip = prog.size ();
    for (auto a : patch)
        prog[a].addr0 = ip - a - 1;
    return true;
}

bool vmcompiler::compile_cat (std::vector<vmcode>& prog)
{
    while (L'|' != mstr[mpos] && L')' != mstr[mpos] && L'\0' != mstr[mpos]) {
        if (! compile_term (prog))
            return false;
    }
    return true;
}

bool vmcompiler::compile_term (std::vector<vmcode>& prog)
{
    std::vector<vmcode> lhs;
    if (! compile_factor (lhs))
        return false;
    int const lhs_size = lhs.size (); // must be int not std::size_t
    if (L'\x3f' == mstr[mpos] || L'*' == mstr[mpos] || L'+' == mstr[mpos]) {
        wchar_t const quorifier = mstr[mpos++];
        bool const greedy = mstr[mpos] != L'\x3f';
        if (! greedy)
            ++mpos;
        if (L'+' == quorifier) {
            if (greedy)
                lhs.push_back (vmcode (vmcode::SPLIT, -(lhs_size + 1), 0));
            else
                lhs.push_back (vmcode (vmcode::SPLIT, 0, -(lhs_size + 1)));
        }
        else {
            int const disp = L'*' == quorifier ? lhs_size + 1 : lhs_size;
            if (greedy)
                lhs.insert (lhs.begin (), vmcode (vmcode::SPLIT, 0, disp));
            else
                lhs.insert (lhs.begin (), vmcode (vmcode::SPLIT, disp, 0));
            if (L'*' == quorifier)
                lhs.push_back (vmcode (vmcode::JMP, -(lhs_size + 2), 0));
        }
    }
    else if (L'{' == mstr[mpos] && ! compile_interval (lhs))
        return false;
    prog.insert (prog.end (), lhs.begin (), lhs.end ());
    return true;
}

bool vmcompiler::compile_interval (std::vector<vmcode>& lhs)
{
    int n1, n2;
    int const mpos0 = mpos++;   // skip L'{'
    if (! digits (n1, 10, 8)) {
        mpos = mpos0;
        return true;
    }
    n2 = n1;
    if (L',' == mstr[mpos]) {
        ++mpos;
        n2 = -1;
        digits (n2, 10, 8);
    }
    if (L'}' != mstr[mpos++]) {
        mpos = mpos0;
        return true;
    }
    if (n2 != -1 && (n1 > n2 || n2 <= 0))
        return false;
    bool const greedy = mstr[mpos] != L'\x3f';
    if (! greedy)
        ++mpos;
    int lhs_size = lhs.size ();
    int reg = mreg++;
    if (greedy)
        lhs.insert (lhs.begin (), vmcode (vmcode::ISPLIT, 0, lhs_size + 1, reg, n1, n2));
    else
        lhs.insert (lhs.begin (), vmcode (vmcode::ISPLIT, lhs_size + 1, 0, reg, n1, n2));
    lhs.insert (lhs.begin (), vmcode (vmcode::RESET, 0, 0, reg, 0, 0));
    lhs.push_back (vmcode (vmcode::JMP, -(lhs_size + 2), 0));
    return true;
}

bool vmcompiler::compile_factor (std::vector<vmcode>& prog)
{
    static const std::wstring pat1 (L".^$");
    static const std::vector<vmcode::operation> op1{
        vmcode::ANY, vmcode::BOL, vmcode::EOL};
    static const std::wstring pat2 (L"ABbz");
    static const std::vector<vmcode::operation> op2{
        vmcode::BOS, vmcode::NWORDB, vmcode::WORDB, vmcode::EOS};
    wchar_t ch;
    std::wstring::size_type idx;
    if (L'(' == mstr[mpos]) {
        int const skip = mstr.compare (mpos, 3, L"(\x3f:") == 0 ? 3 : 1;
        mpos += skip;
        int const n = mgroup + 1;
        if (skip == 1)
            prog.push_back (vmcode (vmcode::SAVE, (++mgroup) * 2, 0));
        if (! compile_exp (prog) || L')' != mstr[mpos++])
            return false;
        if (skip == 1)
            prog.push_back (vmcode (vmcode::SAVE, n * 2 + 1, 0));
    }
    else if (L'[' == mstr[mpos]) {
        ++mpos;
        if (! compile_cclass (prog))
            return false;
    }
    else if ((idx = pat1.find (mstr[mpos])) != std::wstring::npos) {
        prog.push_back (vmcode (op1[idx]));
        ++mpos;
    }
    else if (L'\\' == mstr[mpos]
            && (idx = pat2.find (mstr[mpos + 1])) != std::wstring::npos) {
        prog.push_back (vmcode (op2[idx]));
        mpos += 2;
    }
    else if (L'\x3f' == mstr[mpos] || L'*' == mstr[mpos] || L'+' == mstr[mpos])
        return false;
    else if (compile_char (ch))
        prog.push_back (vmcode (vmcode::CHAR, ch));
    else
        return false;
    return true;
}

bool vmcompiler::compile_cclass (std::vector<vmcode>& prog)
{
    vmcode::operation op = L'^' == mstr[mpos] ? vmcode::NCCLASS : vmcode::CCLASS;
    if (L'^' == mstr[mpos])
        ++mpos;
    auto spanlist = std::make_shared<std::vector<vmspan>> ();
    std::wstring spanstr;
    wchar_t ch, last;
    if (! compile_char (ch))    // L"[]a]" L"[-a]" trick
        return false;
    spanstr.push_back (ch);
    while (L']' != mstr[mpos])
        if (L'-' != mstr[mpos]) {
            if (! compile_char (ch))
                return false;
            spanstr.push_back (ch);
        }
        else if (L']' == mstr[mpos + 1])    // L"[a-]" trick
            spanstr.push_back (mstr[mpos++]);
        else {  // in here, L"[*--]" != L"[*+,-]", it is error.
            ++mpos;     // skip L'-'
            if (L'-' == mstr[mpos] || ! compile_char (last))
                return false;
            spanstr.pop_back ();
            if (spanstr.size () > 0)
                spanlist->push_back (vmspan (spanstr));
            spanstr.clear ();
            spanlist->push_back (vmspan (ch, last));
        }
    ++mpos;     // skip L']'
    if (spanstr.size () > 0)
        spanlist->push_back (vmspan (spanstr));
    prog.push_back (vmcode (op, spanlist));
    return true;
}

bool vmcompiler::compile_char (wchar_t& ch)
{
    std::wstring::size_type idx;
    static const std::wstring pat1 (L"aeftnr");
    static const std::wstring val1 (L"\x07\x1b\f\t\n\r");
    if ((L'\0' <= mstr[mpos] && mstr[mpos] <= '\x1f') || '\x7f' == mstr[mpos])
        return false;
    ch = mstr[mpos++];
    if (L'\\' != ch)
        return true;
    if ((L'\0' <= mstr[mpos] && mstr[mpos] <= '\x1f') || '\x7f' == mstr[mpos])
        return false;
    ch = mstr[mpos++];
    if ((idx = pat1.find (ch)) != std::wstring::npos)
        ch = val1[idx];
    else if (c7toi (ch) < 8) {
        --mpos;
        if (! digits (ch, 8, 3))
            return false;
    }
    else if (L'x' == ch) {
        if (L'{' == mstr[mpos]) {
            ++mpos;
            if (! digits (ch, 16, 8) || L'}' != mstr[mpos++])
                return false;
        }
        else if (! digits (ch, 16, 2))
            return false;
    }
    return true;
}

template<typename T>
bool vmcompiler::digits (T& value, int const base, int const len)
{
    if (c7toi (mstr[mpos]) >= base)
        return false;
    value = 0;
    for (int i = 0; i < len && c7toi (mstr[mpos]) < base; i++)
        value = value * base + c7toi (mstr[mpos++]);
    return true;
}

int vmcompiler::c7toi (wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9') ? ch - L'0'
          : (ch >= L'a' && ch <= L'z') ? ch - L'a' + 10
          : (ch >= L'A' && ch <= L'Z') ? ch - L'A' + 10
          : 36;
}

}//namespace wpike

wregex::wregex (std::wstring s)
{
    wpike::vmcompiler vc;
    if (! vc.compile (s, prog))
        throw regex_error ();
}

}//namespace t42
