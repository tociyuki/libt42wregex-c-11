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
    bool compile_exp (std::vector<vmcode>& prog);
    bool compile_cat (std::vector<vmcode>& prog);
    bool compile_term (std::vector<vmcode>& prog);
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
        std::size_t lhs_size = lhs.size ();
        std::size_t rhs_size = rhs.size ();
        if (lhs_size == 0 && rhs_size == 0)
            continue;
        prog.insert (prog.end (), vmcode (vmcode::SPLIT, 0, lhs_size + 1));
        prog.insert (prog.end (), lhs.begin (), lhs.end ());
        patch.push_back (prog.size ());
        prog.push_back (vmcode (vmcode::JMP, 0, 0));
        lhs = std::move (rhs);
    }
    prog.insert (prog.end (), lhs.begin (), lhs.end ());
    std::size_t ip = prog.size ();
    for (auto a : patch)
        prog[a].addr0 = ip - a - 1;
    return true;
}

bool vmcompiler::compile_cat (std::vector<vmcode>& prog)
{
    if (! compile_term (prog))
        return false;
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
    int lhs_size = lhs.size (); // must be int not std::size_t
    if (L'\x3f' == mstr[mpos] || L'*' == mstr[mpos] || L'+' == mstr[mpos]) {
        wchar_t quorifier = mstr[mpos++];
        bool greedy = mstr[mpos] != L'\x3f';
        if (! greedy)
            ++mpos;
        if (L'\x3f' == quorifier) {
            if (greedy)
                lhs.insert (lhs.begin (), vmcode (vmcode::SPLIT, 0, lhs_size));
            else
                lhs.insert (lhs.begin (), vmcode (vmcode::SPLIT, lhs_size, 0));
        }
        else if (L'*' == quorifier) {
            if (greedy)
                lhs.insert (lhs.begin (), vmcode (vmcode::SPLIT, 0, lhs_size + 1));
            else
                lhs.insert (lhs.begin (), vmcode (vmcode::SPLIT, lhs_size + 1, 0));
            lhs.push_back (vmcode (vmcode::JMP, -(lhs_size + 2), 0));
        }
        else if (L'+' == quorifier) {
            if (greedy)
                lhs.push_back (vmcode (vmcode::SPLIT, 0, -(lhs_size + 1)));
            else
                lhs.push_back (vmcode (vmcode::SPLIT, -(lhs_size + 1), 0));
        }
    }
    prog.insert (prog.end (), lhs.begin (), lhs.end ());
    return true;
}

bool vmcompiler::compile_factor (std::vector<vmcode>& prog)
{
    wchar_t ch;
    std::wstring::size_type idx;
    static const std::wstring pat1 (L".^$");
    static const std::vector<vmcode::operation> op1{
        vmcode::ANY, vmcode::BOL, vmcode::EOL};
    static const std::wstring pat2 (L"ABbz");
    static const std::vector<vmcode::operation> op2{
        vmcode::BOS, vmcode::NWORDB, vmcode::WORDB, vmcode::EOS};

    if (L'(' == mstr[mpos]) {
        int skip = mstr.compare (mpos, 3, L"(\x3f:") == 0 ? 3 : 1;
        mpos += skip;
        int n = mgroup + 1;
        if (skip == 1)
            prog.push_back (vmcode (vmcode::SAVE, (++mgroup) * 2, 0));
        if (! compile_exp (prog))
            return false;
        if (skip == 1)
            prog.push_back (vmcode (vmcode::SAVE, n * 2 + 1, 0));
        if (L')' != mstr[mpos++])
            return false;
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
    else if (compile_char (ch)) {
        prog.push_back (vmcode (vmcode::CHAR, ch));
    }
    else {
        return false;
    }
    return true;
}

bool vmcompiler::compile_cclass (std::vector<vmcode>& prog)
{
    wchar_t ch, last;
    vmcode::operation op = L'^' == mstr[mpos] ? vmcode::NCCLASS : vmcode::CCLASS;
    if (L'^' == mstr[mpos])
        ++mpos;
    auto spanlist = std::make_shared<std::vector<vmspan>> ();
    int count = -1;
    std::wstring spanstr;
    while (L'\0' != mstr[mpos]) {
        ++count;
        if (count && L']' == mstr[mpos])
            break;
        if (count && L'-' == mstr[mpos]
                && (L'\0' == mstr[mpos + 1] || L']' != mstr[mpos + 1]))
            return false;
        if (! compile_char (ch))
            return false;
        spanstr.push_back (ch);
        if (L'-' != mstr[mpos] || L'\0' == mstr[mpos] || L']' == mstr[mpos + 1])
            continue;
        spanstr.pop_back ();
        if (spanstr.size () > 0)
            spanlist->push_back (vmspan (spanstr));
        ++mpos;
        if (L'-' == mstr[mpos] || L'\0' == mstr[mpos])
            return false;
        if (! compile_char (last))
            return false;
        spanlist->push_back (vmspan (ch, last));
        spanstr.clear ();
    }
    if (spanstr.size () > 0)
        spanlist->push_back (vmspan (spanstr));
    if (L']' != mstr[mpos])
        return false;
    ++mpos;
    prog.push_back (vmcode (op, spanlist));
    return true;
}

bool vmcompiler::compile_char (wchar_t& ch)
{
    std::wstring::size_type idx;
    static const std::wstring pat1 (L"aetnr");
    static const std::wstring val1 (L"\x07\x1b\t\n\r");

    if (L'\0' == mstr[mpos])
        return false;
    if (L'\\' != mstr[mpos]) {
        ch = mstr[mpos++];
        return true;
    }
    ++mpos;
    if ((idx = pat1.find (mstr[mpos])) != std::wstring::npos) {
        ch = val1[idx];
    }
    else if (c7toi (mstr[mpos]) < 8) {
        if (! digits (ch, 8, 3))
            return false;
    }
    else if (L'x' == mstr[mpos]) {
        ++mpos;
        if (L'{' == mstr[mpos]) {
            ++mpos;
            if (! digits (ch, 16, 8))
                return false;
            if (L'}' != mstr[mpos++])
                return false;
        }
        else if (! digits (ch, 16, 2))
            return false;
    }
    else if ((mstr[mpos] < L'\0' || L'\x1f' < mstr[mpos]) && '\x7f' != mstr[mpos]) {
        ch = mstr[mpos++];
    }
    else
        return false;
    return true;
}

template<typename T>
bool vmcompiler::digits (T& value, int const base, int const len)
{
    if (c7toi (mstr[mpos]) >= base)
        return false;
    T n = 0;
    for (int i = 0; i < len && c7toi (mstr[mpos]) < base; i++)
        n = n * base + c7toi (mstr[mpos++]);
    value = n;
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
