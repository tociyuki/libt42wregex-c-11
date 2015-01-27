#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

bool vmspan::member (wchar_t const c) const
{
    if (STR == type) {
        if (str.find (c) != std::wstring::npos)
            return true;
    }
    else if (RANGE == type) {
        if (str[0] <= c && c <= str[1])
            return true;
    }
    return false;
}

typedef std::vector<std::wstring::size_type> vmcapture;
typedef std::shared_ptr<vmcapture> vmcap_ptr;
typedef std::shared_ptr<std::vector<int>> vmcnt_ptr;

struct vmthread {
    std::size_t ip;
    std::wstring::size_type sp;
    vmcap_ptr cap;
    vmcnt_ptr cnt;

    vmcap_ptr setcap (std::size_t const i, std::wstring::size_type const sp) const
    {
        vmcap_ptr u = std::make_shared<vmcapture> (cap->begin (), cap->end ());
        if (u->size () < i + 1)
            u->resize (i + 1, std::wstring::npos);
        (*u)[i] = sp;
        return u;
    }

    vmcnt_ptr setcnt (std::size_t const i, int const c) const
    {
        vmcnt_ptr u = std::make_shared<std::vector<int>> (cnt->begin (), cnt->end ());
        if (u->size () < i + 1)
            u->resize (i + 1, 0);
        (*u)[i] = c;
        return u;
    }
};

class vmengine {
public:
    enum { PROGSTART = 0 };
    std::wstring::size_type exec (std::vector<vmcode> const& prog,
        std::wstring const& str, vmcapture& capture,
        std::wstring::size_type const pos);
private:
    typedef std::vector<vmthread> thread_que;
    bool mepsilon;
    std::wstring::size_type advance (std::vector<vmcode> const& prog,
        std::wstring const& str, vmcap_ptr& cap, vmcnt_ptr& cnt,
        std::size_t const start, std::wstring::size_type const pos);
    std::wstring::size_type subadvance (std::vector<vmcode> const& prog,
        std::wstring const& str, vmcap_ptr& cap, vmcnt_ptr& cnt,
        std::size_t const ip, std::wstring::size_type const sp);
    bool iswordboundary (std::wstring const& str, std::wstring::size_type const sp) const;
    bool isword (wchar_t const c) const;
    bool incclass (wchar_t const c, std::shared_ptr<std::vector<vmspan>> const& cclass) const;
    std::size_t check_backref (std::wstring const& str,
        std::wstring::size_type const sp, vmcap_ptr const& cap, int const n);
    void addthread (thread_que& rdy, std::size_t const ip,
        std::wstring::size_type const sp, vmcap_ptr const cap, vmcnt_ptr const cnt);
    void addepsilon (thread_que& rdy, std::size_t const ip,
        std::wstring::size_type const sp, vmcap_ptr const cap, vmcnt_ptr const cnt);
};

bool vmengine::isword (wchar_t const c) const
{
    static const std::wstring words (
        L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
    return words.find (c) != std::wstring::npos;
}

bool vmengine::iswordboundary (std::wstring const& str, std::wstring::size_type const sp) const
{
    if (sp == 0)
        return sp < str.size () && isword (str[sp]);
    else if (sp >= str.size ())
        return sp - 1 < str.size () && isword (str[sp - 1]);
    else if (isword (str[sp - 1]) ^ isword (str[sp]))
        return true;
    else if (sp + 1 < str.size() && (isword (str[sp]) ^ isword (str[sp + 1])))
        return true;
    return false;
}

bool vmengine::incclass (wchar_t const c, std::shared_ptr<std::vector<vmspan>> const& cclass) const
{
    for (auto const span : *cclass)
        if (span.member (c))
            return true;
    return false;
}

void vmengine::addthread (vmengine::thread_que& rdy, std::size_t const ip,
    std::wstring::size_type const sp, vmcap_ptr const cap, vmcnt_ptr const cnt)
{
    for (auto th : rdy)
        if (th.ip == ip && th.sp == sp) {
            th.cap = cap;
            th.cnt = cnt;
            return;
        }
    rdy.push_back (vmthread{ip, sp, cap, cnt});
}

void vmengine::addepsilon (vmengine::thread_que& rdy, std::size_t const ip,
    std::wstring::size_type const sp, vmcap_ptr const cap, vmcnt_ptr const cnt)
{
    mepsilon = true;
    addthread (rdy, ip, sp, cap, cnt);
}

std::size_t vmengine::check_backref (std::wstring const& str,
    std::wstring::size_type const sp, vmcap_ptr const& cap, int const n)
{
    if (sp >= str.size () || n < 0 || n * 2 + 1 >= cap->size ())
        return 0;
    int const i1 = cap->at (n * 2);
    int const i2 = cap->at (n * 2 + 1);
    if (i1 < 0 || i1 >= i2)
        return 0;
    std::wstring gstr (str.begin () + i1, str.begin () + i2);
    return str.compare (sp, i2 - i1, gstr) == 0 ? i2 - i1 : 0;
}

std::wstring::size_type vmengine::exec (std::vector<vmcode> const& prog,
    std::wstring const& str, vmcapture& capture, std::wstring::size_type const pos)
{
    vmcap_ptr cap0 = std::make_shared<vmcapture> ();
    vmcnt_ptr cnt0 = std::make_shared<std::vector<int>> ();
    cap0->push_back (pos);
    cap0->push_back (pos);
    std::wstring::size_type idx = advance (prog, str, cap0, cnt0, PROGSTART, pos);
    capture.clear ();
    capture.insert (capture.begin (), cap0->begin (), cap0->end ());
    return idx;
}

std::wstring::size_type vmengine::subadvance (std::vector<vmcode> const& prog,
    std::wstring const& str, vmcap_ptr& cap, vmcnt_ptr& cnt,
    std::size_t const ip, std::wstring::size_type const sp)
{
    vmengine sub;
    return sub.advance (prog, str, cap, cnt, ip, sp);
}

std::wstring::size_type vmengine::advance (std::vector<vmcode> const& prog,
    std::wstring const& str, vmcap_ptr& cap0, vmcnt_ptr& cnt0,
    std::size_t const start, std::wstring::size_type const pos)
{
    std::wstring::size_type match = std::wstring::npos;
    thread_que run;
    thread_que rdy;
    addthread (run, start, pos, cap0, cnt0);
    for (std::wstring::size_type sp = pos; ; ++sp) {
        mepsilon = true;
        while (mepsilon) {
            mepsilon = false;
            for (auto const th : run) {
                vmcap_ptr cap;
                vmcnt_ptr cnt;
                std::wstring::size_type submatch;
                int n;
                if (sp < th.sp) {
                    addthread (rdy, th.ip, th.sp, th.cap, th.cnt);
                    continue;
                }
                vmcode const op = prog[th.ip];
                switch (op.opcode) {
                case vmcode::CHAR:
                    if (sp < str.size () && str[sp] == op.ch)
                        addthread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::ANY:
                    if (sp < str.size ())
                        addthread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::CCLASS:
                    if (sp < str.size () && incclass (str[sp], op.span))
                        addthread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::NCCLASS:
                    if (sp < str.size () && ! incclass (str[sp], op.span))
                        addthread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::BKREF:
                    n = check_backref (str, sp, th.cap, op.addr0);
                    if (n > 0)
                        addthread (rdy, th.ip + 1, sp + n, th.cap, th.cnt);
                    break;
                case vmcode::BOL:
                    if (sp == 0 || str[sp - 1] == L'\n')
                        addepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::EOL:
                    if (sp >= str.size () || str[sp] == L'\n')
                        addepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::BOS:
                    if (sp == 0)
                        addepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::EOS:
                    if (sp >= str.size ())
                        addepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::WORDB:
                    if (iswordboundary (str, sp))
                        addepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::NWORDB:
                    if (! iswordboundary (str, sp))
                        addepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::SAVE:
                    cap = th.setcap (op.addr0, sp);
                    addepsilon (rdy, th.ip + 1, sp, cap, th.cnt);
                    break;
                case vmcode::MATCH:
                    match = th.sp;
                    cap0 = th.setcap (1, th.sp);
                    cnt0 = th.cnt;
                    goto cutoff_lower_order_threads;
                case vmcode::JMP:
                    addepsilon (rdy, op.addr0 + th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::SPLIT:
                    addepsilon (rdy, op.addr0 + th.ip + 1, sp, th.cap, th.cnt);
                    addepsilon (rdy, op.addr1 + th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::RESET:
                    cnt = th.setcnt (op.reg, 0);
                    addepsilon (rdy, th.ip + 1, sp, th.cap, cnt);
                    break;
                case vmcode::ISPLIT:
                    n = th.cnt->at (op.reg) + 1;
                    cnt = th.setcnt (op.reg, n);
                    if (n <= op.n1)
                        addepsilon (rdy, th.ip + 1, sp, th.cap, cnt);
                    else if (op.n2 == -1 || n <= op.n2) {
                        addepsilon (rdy, op.addr0 + th.ip + 1, sp, th.cap, cnt);
                        addepsilon (rdy, op.addr1 + th.ip + 1, sp, th.cap, cnt);
                    }
                    break;
                case vmcode::LKAHEAD:
                case vmcode::NLKAHEAD:
                    cap = std::make_shared<vmcapture> (th.cap->begin (), th.cap->end ());
                    cnt = std::make_shared<std::vector<int>> (th.cnt->begin (), th.cnt->end ());
                    submatch = subadvance (prog, str, cap, cnt, op.addr0 + th.ip + 1, sp);
                    if ((submatch == std::wstring::npos) ^ (op.opcode == vmcode::LKAHEAD))
                        addepsilon (rdy, op.addr1 + th.ip + 1, sp, cap, cnt);
                    break;
                default:
                    throw "unknown vmcode operation. compile error?";
                }
            }
        cutoff_lower_order_threads:
            std::swap (run, rdy);
            rdy.clear ();
        }
        if (sp >= str.size ())
            break;
    }
    return match;
}

}//namespace wpike

std::wstring::size_type wregex::exec (std::wstring const str,
    std::vector<std::wstring::size_type>& capture,
    std::wstring::size_type const pos) const
{
    wpike::vmengine vm;
    return vm.exec (prog, str, capture, pos);
}

}//namespace t42
