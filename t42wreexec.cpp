#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "t42wregex.hpp"

namespace t42 {
namespace wpike {

bool vmspan::member (wchar_t const c)
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

struct vmthread {
    int ip;
    int sp;
    std::shared_ptr<std::vector<int>> cap;
    std::shared_ptr<std::vector<int>> cnt;
};

class vmengine {
public:
    int exec (std::vector<vmcode>& prog, std::wstring& str, std::vector<int>& capture,
        std::wstring::size_type const pos);
private:
    typedef std::vector<vmthread> thread_que;
    typedef std::shared_ptr<std::vector<int>> cap_ptr;
    typedef std::shared_ptr<std::vector<int>> cnt_ptr;
    bool mepsilon;
    bool iswordboundary (std::wstring& str, int sp);
    bool isword (wchar_t c);
    bool incclass (wchar_t const c, std::shared_ptr<std::vector<vmspan>>& cclass);
    void rdythread (thread_que* const rdy,
        int const ip, int const sp, cap_ptr& cap, cnt_ptr& cnt);
    void rdyepsilon (thread_que* const rdy,
        int const ip, int const sp, cap_ptr& cap, cnt_ptr& cnt);
    template<typename T>
    std::shared_ptr<std::vector<T>> ptrvec_copy_set(
        std::shared_ptr<std::vector<T>> v, int i, T x, T x0);
};

bool vmengine::isword (wchar_t c)
{
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')
        || (c >= L'0' && c <= L'9') || c == L'_';
}

bool vmengine::iswordboundary (std::wstring& str, int sp)
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

bool vmengine::incclass (wchar_t const c, std::shared_ptr<std::vector<vmspan>>& cclass)
{
    for (auto span : *cclass)
        if (span.member (c))
            return true;
    return false;
}

void vmengine::rdythread (vmengine::thread_que* const rdy,
    int const ip, int const sp, vmengine::cap_ptr& cap, vmengine::cnt_ptr& cnt)
{
    for (auto th : *rdy)
        if (th.ip == ip && th.sp == sp) {
            th.cap = cap;
            th.cnt = cnt;
            return;
        }
    rdy->push_back (vmthread{ip, sp, cap, cnt});
}

void vmengine::rdyepsilon (vmengine::thread_que* const rdy,
    int const ip, int const sp, vmengine::cap_ptr& cap, vmengine::cnt_ptr& cnt)
{
    mepsilon = true;
    rdythread (rdy, ip, sp, cap, cnt);
}

template<typename T>
std::shared_ptr<std::vector<T>> vmengine::ptrvec_copy_set(
    std::shared_ptr<std::vector<T>> v, int i, T x, T x0)
{
    auto u = std::make_shared<std::vector<T>> (v->begin (), v->end ());
    if (i + 1 > u->size ())
        u->resize (i + 1, x0);
    (*u)[i] = x;
    return u;
}

int vmengine::exec (std::vector<vmcode>& prog, std::wstring& str,
    std::vector<int>& capture, std::wstring::size_type const pos)
{
    enum { PROGSTART = 0 };
    int match = ::t42::wregex::notmatch;
    thread_que* run = new thread_que;
    thread_que* rdy = new thread_que;
    cap_ptr cap0 = std::make_shared<std::vector<int>> ();
    cap0->push_back (pos);
    cap0->push_back (pos);
    cnt_ptr cnt0 = std::make_shared<std::vector<int>> ();
    rdythread (run, PROGSTART, pos, cap0, cnt0);
    rdy->clear ();
    for (int sp = pos; ; ++sp) {
        mepsilon = true;
        while (mepsilon) {
            mepsilon = false;
            for (auto th : *run) {
                cap_ptr cap;
                cnt_ptr cnt;
                int n;
                int a0 = prog[th.ip].addr0;
                int a1 = prog[th.ip].addr1;
                if (sp < th.sp) {
                    rdythread (rdy, th.ip, th.sp, th.cap, th.cnt);
                    continue;
                }
                if (vmcode::MATCH == prog[th.ip].opcode) {
                    match = th.sp;
                    cap = ptrvec_copy_set (th.cap, 1, th.sp, -1);
                    capture.clear ();
                    capture.insert (capture.begin (), cap->begin (), cap->end ());
                    break;
                }
                switch (prog[th.ip].opcode) {
                case vmcode::CHAR:
                    if (sp < str.size () && str[sp] == prog[th.ip].ch)
                        rdythread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::ANY:
                    if (sp < str.size ())
                        rdythread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::CCLASS:
                    if (sp < str.size () && incclass (str[sp], prog[th.ip].span))
                        rdythread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::NCCLASS:
                    if (sp < str.size () && ! incclass (str[sp], prog[th.ip].span))
                        rdythread (rdy, th.ip + 1, sp + 1, th.cap, th.cnt);
                    break;
                case vmcode::BKREF:
                    if (sp < str.size () && a0 * 2 + 1 < th.cap->size ()) {
                        int i1 = th.cap->at (a0 * 2);
                        int i2 = th.cap->at (a0 * 2 + 1);
                        if (i1 < 0 || i1 >= i2)
                            break;
                        std::wstring gstr (str.begin () + i1, str.begin () + i2);
                        if (str.compare (sp, i2 - i1, gstr) == 0)
                            rdythread (rdy, th.ip + 1, sp + (i2 - i1), th.cap, th.cnt);
                    }
                    break;
                case vmcode::BOL:
                    if (sp == 0 || str[sp - 1] == L'\n')
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::EOL:
                    if (sp >= str.size () || str[sp] == L'\n')
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::BOS:
                    if (sp == 0)
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::EOS:
                    if (sp >= str.size ())
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::WORDB:
                    if (iswordboundary (str, sp))
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::NWORDB:
                    if (! iswordboundary (str, sp))
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::SAVE:
                    cap = ptrvec_copy_set (th.cap, a0, sp, -1);
                    rdyepsilon (rdy, th.ip + 1, sp, cap, th.cnt);
                    break;
                case vmcode::JMP:
                    rdyepsilon (rdy, a0 + th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::SPLIT:
                    rdyepsilon (rdy, a0 + th.ip + 1, sp, th.cap, th.cnt);
                    rdyepsilon (rdy, a1 + th.ip + 1, sp, th.cap, th.cnt);
                    break;
                case vmcode::RESET:
                    cnt = ptrvec_copy_set (th.cnt, prog[th.ip].reg, 0, 0);
                    rdyepsilon (rdy, th.ip + 1, sp, th.cap, cnt);
                    break;
                case vmcode::ISPLIT:
                    n = th.cnt->at (prog[th.ip].reg) + 1;
                    cnt = ptrvec_copy_set (th.cnt, prog[th.ip].reg, n, 0);
                    if (n <= prog[th.ip].n1)
                        rdyepsilon (rdy, th.ip + 1, sp, th.cap, cnt);
                    else if (prog[th.ip].n2 == -1 || n <= prog[th.ip].n2) {
                        rdyepsilon (rdy, a0 + th.ip + 1, sp, th.cap, cnt);
                        rdyepsilon (rdy, a1 + th.ip + 1, sp, th.cap, cnt);
                    }
                    break;
                default:
                    break;
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

int wregex::exec (std::wstring str, std::vector<int>& capture,
    std::wstring::size_type const pos)
{
    wpike::vmengine vm;
    return vm.exec (prog, str, capture, pos);
}

}//namespace t42
