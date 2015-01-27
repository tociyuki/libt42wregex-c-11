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

typedef std::shared_ptr<std::vector<std::wstring::size_type>> cap_ptr;
typedef std::shared_ptr<std::vector<int>> cnt_ptr;

struct vmthread {
    std::size_t ip;
    std::wstring::size_type sp;
    cap_ptr cap;
    cnt_ptr cnt;
};

class vmengine {
public:
    std::wstring::size_type exec (std::vector<vmcode> const& prog,
        std::wstring const& str, std::vector<std::wstring::size_type>& capture,
        std::wstring::size_type const pos);
private:
    typedef std::vector<vmthread> thread_que;
    bool mepsilon;
    bool iswordboundary (std::wstring const& str, std::wstring::size_type const sp) const;
    bool isword (wchar_t const c) const;
    bool incclass (wchar_t const c, std::shared_ptr<std::vector<vmspan>> const& cclass) const;
    std::size_t check_backref (std::wstring const& str,
        std::wstring::size_type const sp, cap_ptr const& cap, int const n);
    void addthread (thread_que* const rdy, std::size_t const ip,
        std::wstring::size_type const sp, cap_ptr const& cap, cnt_ptr const& cnt);
    void addepsilon (thread_que* const rdy, std::size_t const ip,
        std::wstring::size_type const sp, cap_ptr const& cap, cnt_ptr const& cnt);
    template<typename T>
    std::shared_ptr<std::vector<T>> ptrvec_copy_set(
        std::shared_ptr<std::vector<T>> v, std::size_t i, T x, T x0);
};

bool vmengine::isword (wchar_t const c) const
{
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')
        || (c >= L'0' && c <= L'9') || c == L'_';
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

void vmengine::addthread (vmengine::thread_que* const rdy, std::size_t const ip,
    std::wstring::size_type const sp, cap_ptr const& cap, cnt_ptr const& cnt)
{
    for (auto th : *rdy)
        if (th.ip == ip && th.sp == sp) {
            th.cap = cap;
            th.cnt = cnt;
            return;
        }
    rdy->push_back (vmthread{ip, sp, cap, cnt});
}

void vmengine::addepsilon (vmengine::thread_que* const rdy, std::size_t const ip,
    std::wstring::size_type const sp, cap_ptr const& cap, cnt_ptr const& cnt)
{
    mepsilon = true;
    addthread (rdy, ip, sp, cap, cnt);
}

template<typename T>
std::shared_ptr<std::vector<T>> vmengine::ptrvec_copy_set(
    std::shared_ptr<std::vector<T>> v, std::size_t i, T x, T x0)
{
    auto u = std::make_shared<std::vector<T>> (v->begin (), v->end ());
    if (i + 1 > u->size ())
        u->resize (i + 1, x0);
    (*u)[i] = x;
    return u;
}

std::size_t vmengine::check_backref (std::wstring const& str,
    std::wstring::size_type const sp, cap_ptr const& cap, int const n)
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
    std::wstring const& str, std::vector<std::wstring::size_type>& capture,
    std::wstring::size_type const pos)
{
    enum { PROGSTART = 0 };
    std::wstring::size_type match = std::wstring::npos;
    thread_que* run = new thread_que;
    thread_que* rdy = new thread_que;
    cap_ptr cap0 = std::make_shared<std::vector<std::wstring::size_type>> ();
    cap0->push_back (pos);
    cap0->push_back (pos);
    cnt_ptr cnt0 = std::make_shared<std::vector<int>> ();
    addthread (run, PROGSTART, pos, cap0, cnt0);
    for (std::wstring::size_type sp = pos; ; ++sp) {
        mepsilon = true;
        while (mepsilon) {
            mepsilon = false;
            for (auto const th : *run) {
                cap_ptr cap;
                cnt_ptr cnt;
                int n;
                if (sp < th.sp) {
                    addthread (rdy, th.ip, th.sp, th.cap, th.cnt);
                    continue;
                }
                vmcode const op = prog[th.ip];
                if (vmcode::MATCH == op.opcode) {
                    match = th.sp;
                    cap = ptrvec_copy_set (th.cap, 1, th.sp, std::wstring::npos);
                    capture.clear ();
                    capture.insert (capture.begin (), cap->begin (), cap->end ());
                    break;
                }
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
                    cap = ptrvec_copy_set (th.cap, op.addr0, sp, std::wstring::npos);
                    addepsilon (rdy, th.ip + 1, sp, cap, th.cnt);
                    break;
                case vmcode::JMP:
                    addepsilon (rdy, op.addr0 + th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::SPLIT:
                    addepsilon (rdy, op.addr0 + th.ip + 1, sp, th.cap, th.cnt);
                    addepsilon (rdy, op.addr1 + th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::RESET:
                    cnt = ptrvec_copy_set (th.cnt, op.reg, 0, 0);
                    addepsilon (rdy, th.ip + 1, sp, th.cap, cnt);
                    break;
                case vmcode::ISPLIT:
                    n = th.cnt->at (op.reg) + 1;
                    cnt = ptrvec_copy_set (th.cnt, op.reg, n, 0);
                    if (n <= op.n1)
                        addepsilon (rdy, th.ip + 1, sp, th.cap, cnt);
                    else if (op.n2 == -1 || n <= op.n2) {
                        addepsilon (rdy, op.addr0 + th.ip + 1, sp, th.cap, cnt);
                        addepsilon (rdy, op.addr1 + th.ip + 1, sp, th.cap, cnt);
                    }
                    break;
                default:
                    throw "unknown vmcode operation. compile error?";
                }
            }
            std::swap (run, rdy);
            rdy->clear ();
        }
        if (sp >= str.size ())
            break;
    }
    delete run;
    delete rdy;
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
