#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

typedef std::size_t instruction_pointer;
typedef std::wstring::size_type string_pointer;
typedef std::shared_ptr<capture_list> capture_ptr;
typedef std::vector<int> counter_list;
typedef std::shared_ptr<counter_list> counter_ptr;

struct vmthread {
    instruction_pointer ip;
    capture_ptr cap;
    counter_ptr cnt;

    capture_ptr update (std::size_t const i, string_pointer const x) const
    {
        capture_ptr u = std::make_shared<capture_list> (cap->begin (), cap->end ());
        if (u->size () < i + 1)
            u->resize (i + 1, std::wstring::npos);
        (*u)[i] = x;
        return u;
    }

    counter_ptr preset (std::size_t const i, int const x) const
    {
        counter_ptr u = std::make_shared<counter_list> (cnt->begin (), cnt->end ());
        if (u->size () < i + 1)
            u->resize (i + 1, 0);
        (*u)[i] = x;
        return u;
    }
};

typedef std::vector<vmthread> vmthread_que;

class epsilon_closure {
public:
    epsilon_closure (program const& e0, std::wstring const& s0)
        : e (e0), s (s0), gen (1), mark (e0.size (), 0) {}
    bool advance (vmthread& th0, string_pointer sp0, int d);
private:
    program const& e;
    std::wstring const& s;
    int gen;
    std::vector<int> mark;
    void addthread (vmthread_que& q, vmthread&& th, string_pointer const sp, int d);
    bool cclass (std::wstring const& span, wchar_t const c) const;
    bool atwordbound (string_pointer const sp) const;
    bool isword (wchar_t const c) const;
    int backref (vmthread const& th, string_pointer const sp, int d) const;
};

// based on Russ Cox, ``Regular Expression Matching: the Virtual Machine Approach''
//      http://swtch.com/~rsc/regexp/regexp2.html
bool epsilon_closure::advance (vmthread& th0, string_pointer const sp0, int d)
{
    string_pointer match = false;
    vmthread_que run, rdy;
    addthread (run, vmthread{th0.ip, th0.cap, th0.cnt}, sp0, d);
    for (string_pointer sp = sp0; ; sp += d) {
        if (run.empty ())
            break;
        ++gen;
        //  d > 0   "abc"."d"_"efg"     s[sp] == op.s[0]
        //  d < 0   "abc"_"d"."efg"     s[sp-1] == op.s[0]
        string_pointer sp1 = d > 0 ? sp : sp - 1;
        for (vmthread th : run) {
            int ct;
            instruction op = e[th.ip];
            switch (op.opcode) {
            case CHAR:
                if (sp1 < s.size () && s[sp1] == op.s[0])
                    addthread (rdy, vmthread{th.ip + 1, th.cap, th.cnt}, sp + d, d);
                break;
            case ANY:
                if (sp1 < s.size ())
                    addthread (rdy, vmthread{th.ip + 1, th.cap, th.cnt}, sp + d, d);
                break;
            case CCLASS:
            case NCCLASS:
                if (sp1 < s.size () && (cclass (op.s, s[sp1]) ^ (op.opcode == NCCLASS)))
                    addthread (rdy, vmthread{th.ip + 1, th.cap, th.cnt}, sp + d, d);
                break;
            case BKREF:
                ct = backref (th, sp1, d);
                if (ct > 0)
                    addthread (rdy, vmthread{th.ip, th.cap, th.preset (op.r, ct)}, sp + d, d);
                else if (ct == 0)
                    addthread (rdy, vmthread{th.ip + 1, th.cap, th.cnt}, sp + d, d);
                break;
            case MATCH:
                th0.cap = th.update (1, sp);
                match = true;
                goto cutoff_lower_order_threads;
            default:
                throw "JMP, SPLIT, SAVE, and so on already with addthread.. but why?";
            }
        }
    cutoff_lower_order_threads:
        std::swap (run, rdy);
        rdy.clear ();
        if (sp1 >= s.size ())
            break;
    }
    return match;
}

void epsilon_closure::addthread (vmthread_que& q, vmthread&& th, string_pointer const sp, int d)
{
    if (mark[th.ip] == gen)
        return;
    mark[th.ip] = gen;
    instruction op = e[th.ip];
    switch (op.opcode) {
    default:
        q.push_back (std::move (th));
        break;
    case BOL:
        if (sp - 1 >= s.size () || L'\n' == s[sp - 1])
            addthread (q, vmthread{th.ip + 1, th.cap, th.cnt}, sp, d);
        break;
    case EOL:
        if (sp >= s.size () || L'\n' == s[sp])
            addthread (q, vmthread{th.ip + 1, th.cap, th.cnt}, sp, d);
        break;
    case BOS:
        if (sp == 0)
            addthread (q, vmthread{th.ip + 1, th.cap, th.cnt}, sp, d);
        break;
    case EOS:
        if (sp >= s.size ())
            addthread (q, vmthread{th.ip + 1, th.cap, th.cnt}, sp, d);
        break;
    case WORDB:
    case NWORDB:
        if (atwordbound (sp) ^ (NWORDB == op.opcode))
            addthread (q, vmthread{th.ip + 1, th.cap, th.cnt}, sp, d);
        break;
    case LKAHEAD:
    case NLKAHEAD:
        {
            vmthread th1{op.x + th.ip + 1, th.cap, th.cnt};
            if (advance (th1, sp, +1) ^ (NLKAHEAD == op.opcode))
                addthread (q, vmthread{op.y + th.ip + 1, th1.cap, th1.cnt}, sp, d);
        }
        break;
    case LKBEHIND:
    case NLKBEHIND:
        {
            vmthread th1{op.x + th.ip + 1, th.cap, th.cnt};
            if (advance (th1, sp, -1) ^ (NLKBEHIND == op.opcode))
                addthread (q, vmthread{op.y + th.ip + 1, th1.cap, th1.cnt}, sp, d);
        }
        break;
    case RESET:
        addthread (q, vmthread{th.ip + 1, th.cap, th.preset (op.r, 0)}, sp, d);
        break;
    case REP:
        {
            int const i = th.cnt->at (op.r) + 1;
            counter_ptr cnt = th.preset (op.r, i);
            if (i <= op.x)
                addthread (q, vmthread{th.ip + 2, th.cap, cnt}, sp, d);
            else if (op.y == -1 || i <= op.y)
                addthread (q, vmthread{th.ip + 1, th.cap, cnt}, sp, d);
            else if (op.x == op.y)
                addthread (q, vmthread{th.ip + 2 + e[th.ip + 1].y, th.cap, cnt}, sp, d);
        }
        break;
    case JMP:
        addthread (q, vmthread{th.ip + 1 + op.x, th.cap, th.cnt}, sp, d);
        break;
    case SPLIT:
        addthread (q, vmthread{th.ip + 1 + op.x, th.cap, th.cnt}, sp, d);
        addthread (q, vmthread{th.ip + 1 + op.y, th.cap, th.cnt}, sp, d);
        break;
    case SAVE:
        addthread (q, vmthread{th.ip + 1, th.update (op.x, sp), th.cnt}, sp, d);
        break;
    }
}

bool epsilon_closure::cclass (std::wstring const& span, wchar_t const c) const
{
    for (auto p = span.begin (); p < span.end (); ++p) {
        if (L'\\' == *p) {
            if (c == *++p)
                return true;
        }
        else {
            if (p[-1] <= c && c <= p[2])
                return true;
            p += 2;
        }
    }
    return false;
}

bool epsilon_closure::atwordbound (string_pointer const sp) const
{
    wchar_t c0 = sp - 1 < s.size () ? s[sp - 1] : L' ';
    wchar_t c1 = sp < s.size () ? s[sp] : L' ';
    return isword (c0) ^ isword(c1) ? true : false;
}

bool epsilon_closure::isword (wchar_t const c) const
{
    static const std::wstring words (
        L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
    return c >= 128 || words.find (c) != std::wstring::npos;
}

// return > 0   continue backref comparison
// return == 0  match backref comparison
// return < 0   failure backref comparison
int epsilon_closure::backref (vmthread const& th, string_pointer const sp, int d) const
{
    int const r = e[th.ip].r; // counter register number
    int const n = e[th.ip].x; // capture number
    if (sp >= s.size () || n < 0 || n * 2 + 1 >= th.cap->size ())
        return -1;
    int const ct = th.cnt->at (r);
    int const i1 = th.cap->at (n * 2);
    int const i2 = th.cap->at (n * 2 + 1);
    if (i1 < 0 || i1 >= i2 || ct >= i2 - i1)
        return -1;
    // ct     0123456
    // "abc "."backref"_" def"  s[i1 + ct]
    // ct       6543210
    // "abc "_"backref"." def"  s[i2 - ct - 1]
    int const i = d > 0 ? i1 + ct : i2 - ct - 1;
    if (s[sp] != s[i])
        return -1;
    if (ct < i2 - i1 - 1)
        return ct + 1;
    return 0;
}

}//namespace wpike

std::wstring::size_type wregex::exec (std::wstring const s,
    wpike::capture_list& m, std::wstring::size_type const sp) const
{
    enum { START = 0 };
    wpike::epsilon_closure vm (e, s);
    wpike::vmthread th{
        START,
        std::make_shared<wpike::capture_list> (2, sp),
        std::make_shared<wpike::counter_list> ()
    };
    bool x = vm.advance (th, sp, +1);
    m.clear ();
    m.insert (m.begin (), th.cap->begin (), th.cap->end ());
    return x ? m[1] : std::wstring::npos;
}

}//namespace t42
