#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "t42wregex.hpp"

namespace t42 {

namespace wpike {

class vmnode {
public:
    vmnode () {}
    virtual ~vmnode () {}
    void generate (std::vector<vmcode>& prog);
    virtual void generate_step (std::vector<vmcode>& prog) = 0;
};

class vmparser {
public:
    bool parse (std::wstring str, std::shared_ptr<vmnode>& exp);
private:
    std::wstring mstr;
    int mpos;
    int mgroup;
    bool expr (std::shared_ptr<vmnode>& exp);
    bool cat (std::shared_ptr<vmnode>& exp);
    bool term (std::shared_ptr<vmnode>& exp);
    bool factor (std::shared_ptr<vmnode>& exp);
    bool group (std::shared_ptr<vmnode>& exp);
    bool cclass (std::shared_ptr<vmnode>& exp);
    int cclass_char (wchar_t& ch, int mpos0);
    bool cesc (wchar_t& ch);
    bool cescxdigit (wchar_t& ch);
    template<typename T>
    bool digits (T& value, int const base, int const len);
    template<typename T>
    bool translit (wchar_t ch, T& x, std::wstring&& from, std::basic_string<T>&& to);
    bool is7alnum (wchar_t ch);
    int toi7casec (wchar_t ch);
    bool is7casedigit (wchar_t const ch, int const base);
};

class vmalt_node : public vmnode {
public:
    vmalt_node (std::shared_ptr<vmnode>& a, std::shared_ptr<vmnode>& b)
        : lhs (a), rhs (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<vmnode> lhs;
    std::shared_ptr<vmnode> rhs;
};

class vmcat_node : public vmnode {
public:
    vmcat_node (std::shared_ptr<vmnode>& a, std::shared_ptr<vmnode>& b)
        : lhs (a), rhs (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<vmnode> lhs;
    std::shared_ptr<vmnode> rhs;
};

class vmopt_node : public vmnode {
public:
    vmopt_node (std::shared_ptr<vmnode>& a, bool b) : lhs (a), greedy (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<vmnode> lhs;
    bool greedy;
};

class vmmany0_node : public vmnode {
public:
    vmmany0_node (std::shared_ptr<vmnode>& a, bool b) : lhs (a), greedy (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<vmnode> lhs;
    bool greedy;
};

class vmmany1_node : public vmnode {
public:
    vmmany1_node (std::shared_ptr<vmnode>& a, bool b) : lhs (a), greedy (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<vmnode> lhs;
    bool greedy;
};

class vmgroup_node : public vmnode {
public:
    vmgroup_node (std::shared_ptr<vmnode>& a, int b) : lhs (a), num (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<vmnode> lhs;
    int num;
};

class vmchar_node : public vmnode {
public:
    vmchar_node (wchar_t a) : ch (a) { }
    void generate_step (std::vector<vmcode>& prog);
protected:
    wchar_t ch;
};

class vmcclass_node : public vmnode {
public:
    vmcclass_node (std::shared_ptr<std::vector<vmspan>>& a, bool b) : span (a), complement (b) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    std::shared_ptr<std::vector<vmspan>> span;
    bool complement;
};

class vminherit_node : public vmnode {
public:
    vminherit_node (vmcode::operation a) : opcode (a) { }
    void generate_step (std::vector<vmcode>& prog);
private:
    vmcode::operation opcode;
};

bool vmparser::parse (std::wstring str, std::shared_ptr<vmnode>& exp)
{
    mstr = str + L'\0';
    mpos = 0;
    mgroup = 0;
    return expr (exp);
}

bool vmparser::expr (std::shared_ptr<vmnode>& exp)
{
    std::shared_ptr<vmnode> lhs;
    if (! cat (lhs))
        return false;
    while (mstr[mpos] == L'|') {
        ++mpos;
        std::shared_ptr<vmnode> rhs;
        if (! cat (rhs))
            return false;
        lhs = std::make_shared<vmalt_node> (lhs, rhs);
    }
    exp = lhs;
    return true;
}

bool vmparser::cat (std::shared_ptr<vmnode>& exp)
{
    std::shared_ptr<vmnode> lhs;
    if (! term (lhs))
        return false;
    while (mstr[mpos] != L'|' && mstr[mpos] != L')' && mstr[mpos] != L'\0') {
        std::shared_ptr<vmnode> rhs;
        if (! term (rhs))
            return false;
        lhs = std::make_shared<vmcat_node> (lhs, rhs);
    }
    exp = lhs;
    return true;
}

bool vmparser::term (std::shared_ptr<vmnode>& exp)
{
    std::shared_ptr<vmnode> lhs;
    if (! factor (lhs))
        return false;
    if (mstr[mpos] == L'?' || mstr[mpos] == L'*' || mstr[mpos] == L'+') {
        wchar_t q = mstr[mpos++];
        bool greedy = true;
        if (mstr[mpos] == L'?') {
            greedy = false;
            ++mpos;
        }
        if (L'?' == q)
            lhs = std::make_shared<vmopt_node> (lhs, greedy);
        else if (L'*' == q)
            lhs = std::make_shared<vmmany0_node> (lhs, greedy);
        else if (L'+' == q)
            lhs = std::make_shared<vmmany1_node> (lhs, greedy);
    }
    exp = lhs;
    return true;
}

bool vmparser::factor (std::shared_ptr<vmnode>& exp)
{
    wchar_t ch;
    if (mstr[mpos] == L'(') {
        ++mpos;
        if (! group (exp))
            return false;
    }
    else if (mstr[mpos] == L'[') {
        ++mpos;
        if (! cclass (exp))
            return false;
    }
    else if (mstr[mpos] == L'.') {
        exp = std::make_shared<vminherit_node> (vmcode::ANY);
        ++mpos;
    }
    else if (mstr[mpos] == L'^') {
        exp = std::make_shared<vminherit_node> (vmcode::BOL);
        ++mpos;
    }
    else if (mstr[mpos] == L'$') {
        exp = std::make_shared<vminherit_node> (vmcode::EOL);
        ++mpos;
    }
    else if (mstr[mpos] == L'\\') {
        ++mpos;
        if (mstr[mpos] == L'A') {
            exp = std::make_shared<vminherit_node> (vmcode::BOS);
            ++mpos;
        }
        else if (mstr[mpos] == L'z') {
            exp = std::make_shared<vminherit_node> (vmcode::EOS);
            ++mpos;
        }
        else if (mstr[mpos] == L'b') {
            exp = std::make_shared<vminherit_node> (vmcode::WORDB);
            ++mpos;
        }
        else if (mstr[mpos] == L'B') {
            exp = std::make_shared<vminherit_node> (vmcode::NWORDB);
            ++mpos;
        }
        else if (cesc (ch)) {
            exp = std::make_shared<vmchar_node> (ch);
        }
        else {
            return false;
        }
    }
    else {
        exp = std::make_shared<vmchar_node> (mstr[mpos]);
        mpos++;
    }
    return true;
}

bool vmparser::group (std::shared_ptr<vmnode>& exp)
{
    enum {GROUP, SUBEXP} type = SUBEXP;
    int n = 0;
    if (mstr[mpos] == L'?' && mstr[mpos + 1] == L':') {
        mpos += 2;
    }
    else {
        type = GROUP;
        n = ++mgroup;
    }
    std::shared_ptr<vmnode> lhs;
    if (! expr (lhs))
        return false;
    if (mstr[mpos] != L')')
        return false;
    ++mpos;
    if (type == GROUP)
        exp = std::make_shared<vmgroup_node> (lhs, n);
    else
        exp = lhs;
    return true;
}

bool vmparser::cclass (std::shared_ptr<vmnode>& exp)
{
    bool complement = false;
    if (mstr[mpos] == L'^') {
        complement = true;
        ++mpos;
    }
    auto spanlist = std::make_shared<std::vector<vmspan>> ();
    int mpos0 = mpos;
    while (mstr[mpos] != L']') {
        std::wstring spanstr;
        wchar_t ch;
        for (;;) {
            int rc = cclass_char (ch, mpos0);
            if (rc < 0)
                return false;
            else if (rc == 0)
                break;
            spanstr.append (1, ch);
        }
        if (mstr[mpos] == L'-' && spanstr.size () > 0)
            spanstr.pop_back ();
        if (spanstr.size () > 0)
            spanlist->push_back (vmspan (spanstr));
        if (mstr[mpos] == L'-') {
            ++mpos;
            wchar_t last;
            int rc = cclass_char (last, mpos0);
            if (rc <= 0)
                return false;
            if (ch > last)
                return false;
            spanlist->push_back (vmspan (ch, last));
        }
    }
    ++mpos;
    exp = std::make_shared<vmcclass_node> (spanlist, complement);
    return true;
}

int vmparser::cclass_char (wchar_t& ch, int mpos0)
{
    if (mstr[mpos] == L'\0') {
        return -1;
    }
    else if (mstr[mpos] == L'\\') {
        if (! cesc (ch))
            return -1;
    }
    else if (mstr[mpos] == L'-' && mpos == mpos0) {
        ch = mstr[mpos++];
    }
    else if (mstr[mpos] == L'-' && mstr[mpos + 1] == L']') {
        ch = mstr[mpos++];
    }
    else if (mstr[mpos] == L'-' || mstr[mpos] == L']') {
        return 0;
    }
    else {
        ch = mstr[mpos++];
    }
    return 1;
}

bool vmparser::cesc (wchar_t& ch)
{
    switch (mstr[mpos]) {
    case L'x':
        ++mpos;
        if (! cescxdigit (ch))
            return false;
        break;
    default:
        if (translit (mstr[mpos], ch, std::wstring(L"tnrfae"),
                std::wstring(L"\t\n\r\x0c\x07\x1b"))) {
            ++mpos;
        }
        else if (is7casedigit (mstr[mpos], 8)) {
            digits (ch, 8, 3);
        }
        else if (mstr[mpos] >= L' ' && mstr[mpos] != 0x7f) {
            ch = mstr[mpos];
            ++mpos;
        }
        else {
            return false;
        }
    }
    return true;
}

bool vmparser::cescxdigit (wchar_t& ch)
{
    if (is7casedigit (mstr[mpos], 16) && is7casedigit (mstr[mpos + 1], 16)) {
        ch = toi7casec (mstr[mpos]) * 16 + toi7casec (mstr[mpos + 1]);
        mpos += 2;
    }
    else if (mstr[mpos] == L'{') {
        ++mpos;
        if (! digits (ch, 16, 8))
            return false;
        if (mstr[mpos] != L'}')
            return false;
        ++mpos;
    }
    return true;
}

template<typename T>
bool vmparser::digits (T& value, int const base, int const len)
{
    if (! is7casedigit (mstr[mpos], base))
        return false;
    T n = 0;
    for (int i = 0; i < len && is7casedigit (mstr[mpos], base); i++)
        n = n * base + toi7casec (mstr[mpos++]);
    value = n;
    return true;
}

template<typename T>
bool vmparser::translit (wchar_t ch, T& x,
    std::wstring&& from, std::basic_string<T>&& to)
{
    std::wstring::size_type i = from.find (ch);
    if (i == std::wstring::npos)
        return false;
    x = to[i];
    return true;
}

bool vmparser::is7alnum (wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9')
        || (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
}

int vmparser::toi7casec (wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9') ? ch - L'0'
          : (ch >= L'a' && ch <= L'z') ? ch - L'a' + 10
          : (ch >= L'A' && ch <= L'Z') ? ch - L'A' + 10
          : 0;
}

bool vmparser::is7casedigit (wchar_t const ch, int const base)
{
    return is7alnum (ch) && toi7casec (ch) < base;
}

void vmnode::generate (std::vector<vmcode>& prog)
{
    generate_step (prog);
    prog.push_back (vmcode (vmcode::MATCH));
}

void vmcat_node::generate_step (std::vector<vmcode>& prog)
{
    lhs->generate_step (prog);
    rhs->generate_step (prog);
}

void vmalt_node::generate_step (std::vector<vmcode>& prog)
{
    int ip0 = prog.size ();
    prog.push_back (vmcode (vmcode::SPLIT, ip0 + 1, ip0 + 1));
    lhs->generate_step (prog);
    int ip1 = prog.size ();
    prog[ip0].addr1 = ip1 + 1;
    prog.push_back (vmcode (vmcode::JMP, ip1 + 1));
    rhs->generate_step (prog);
    prog[ip1].addr0 = prog.size ();
}

void vmopt_node::generate_step (std::vector<vmcode>& prog)
{
    int ip0 = prog.size ();
    prog.push_back (vmcode (vmcode::SPLIT, ip0 + 1, ip0 + 1));
    lhs->generate_step (prog);
    if (greedy)
        prog[ip0].addr1 = prog.size ();
    else
        prog[ip0].addr0 = prog.size ();
}

void vmmany0_node::generate_step (std::vector<vmcode>& prog)
{
    int ip0 = prog.size ();
    prog.push_back (vmcode (vmcode::SPLIT, ip0 + 1, ip0 + 1));
    lhs->generate_step (prog);
    prog.push_back (vmcode (vmcode::JMP, ip0));
    if (greedy)
        prog[ip0].addr1 = prog.size ();
    else
        prog[ip0].addr0 = prog.size ();
}

void vmmany1_node::generate_step (std::vector<vmcode>& prog)
{
    int ip0 = prog.size ();
    lhs->generate_step (prog);
    int ip1 = prog.size ();
    if (greedy)
        prog.push_back (vmcode (vmcode::SPLIT, ip0, ip1 + 1));
    else
        prog.push_back (vmcode (vmcode::SPLIT, ip1 + 1, ip0));
}

void vmgroup_node::generate_step (std::vector<vmcode>& prog)
{
    prog.push_back (vmcode (vmcode::SAVE, num * 2));
    lhs->generate_step (prog);
    prog.push_back (vmcode (vmcode::SAVE, num * 2 + 1));
}

void vmchar_node::generate_step (std::vector<vmcode>& prog)
{
    prog.push_back (vmcode (vmcode::CHAR, ch));
}

void vmcclass_node::generate_step (std::vector<vmcode>& prog)
{
    vmcode::operation opcode = complement ? vmcode::NCCLASS : vmcode::CCLASS;
    prog.push_back (vmcode (opcode, span));
}

void vminherit_node::generate_step (std::vector<vmcode>& prog)
{
    prog.push_back (vmcode (opcode));
}

}//namespace wpike

wregex::wregex (std::wstring s) throw (regex_error)
{
    wpike::vmparser syn;
    std::shared_ptr<wpike::vmnode> exp;
    if (! syn.parse (s, exp))
        throw regex_error ();
    exp->generate (prog);
}

}//namespace t42

