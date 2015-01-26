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
    std::shared_ptr<std::vector<int>> capture;
    std::shared_ptr<std::vector<int>> counter;
    vmthread (int a, int b,
            std::shared_ptr<std::vector<int>> const& c,
            std::shared_ptr<std::vector<int>> const& d)
        : ip (a), sp (b), capture (c), counter (d) { }
};

class vmengine {
public:
    int exec (std::vector<vmcode>& prog, std::wstring& str, std::vector<int>& capture,
        std::wstring::size_type const pos);
private:
    bool iswordboundary (std::wstring& str, int sp);
    bool isword (wchar_t c);
    bool incclass (wchar_t const c, std::shared_ptr<std::vector<vmspan>>& cclass);
    void runthread (
        std::vector<vmthread>* const que, int const from,
        int const ip, int const sp,
        std::shared_ptr<std::vector<int>>& capture,
        std::shared_ptr<std::vector<int>>& counter);
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

void vmengine::runthread (
    std::vector<vmthread>* const que, int const from, int const ip, int const sp,
    std::shared_ptr<std::vector<int>>& capture,
    std::shared_ptr<std::vector<int>>& counter)
{
    for (auto p = que->begin () + from; p != que->end (); ++p)
        if (p->ip == ip && p->sp == sp) {
            p->capture = capture;
            p->counter = counter;
            return;
        }
    vmthread th (ip, sp, capture, counter);
    que->push_back (th);
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
    auto run = new std::vector<vmthread>;
    auto rdy = new std::vector<vmthread>;
    auto cap0 = std::make_shared<std::vector<int>> ();
    cap0->push_back (pos);
    cap0->push_back (pos);
    auto cnt0 = std::make_shared<std::vector<int>> ();
    runthread (run, 0, PROGSTART, pos, cap0, cnt0);
    rdy->clear ();
    for (int sp = pos; ; sp++) {
        for (int th = 0; th < run->size(); th++) {
            int ip = run->at (th).ip;
            int a0 = prog[ip].addr0;
            int a1 = prog[ip].addr1;
            auto cap = run->at (th).capture;
            auto cnt = run->at (th).counter;
            if (sp < run->at (th).sp && run->at (th).sp < str.size ()) {
                runthread (rdy, 0, ip, run->at (th).sp, cap, cnt);
                continue;
            }
            if (vmcode::MATCH == prog[ip].opcode) {
                match = sp;
                capture.clear ();
                capture.insert (capture.begin (), cap->begin (), cap->end ());
                capture[1] = sp;
                break;
            }
            switch (prog[ip].opcode) {
            case vmcode::CHAR:
                if (sp < str.size () && str[sp] == prog[ip].ch)
                    runthread (rdy, 0, ip + 1, sp + 1, cap, cnt);
                break;
            case vmcode::ANY:
                if (sp < str.size ())
                    runthread (rdy, 0, ip + 1, sp + 1, cap, cnt);
                break;
            case vmcode::CCLASS:
                if (sp < str.size () && incclass (str[sp], prog[ip].span))
                    runthread (rdy, 0, ip + 1, sp + 1, cap, cnt);
                break;
            case vmcode::NCCLASS:
                if (sp < str.size () && ! incclass (str[sp], prog[ip].span))
                    runthread (rdy, 0, ip + 1, sp + 1, cap, cnt);
                break;
            case vmcode::BKREF:
                if (sp < str.size () && a0 * 2 + 1 < cap->size ()) {
                    int i1 = (*cap)[a0 * 2];
                    int i2 = (*cap)[a0 * 2 + 1];
                    if (i1 < 0 || i1 >= i2)
                        break;
                    std::wstring gstr (str.begin () + i1, str.begin () + i2);
                    if (str.compare (sp, i2 - i1, gstr) == 0)
                        runthread (rdy, 0, ip + 1, sp + (i2 - i1), cap, cnt);
                }
                break;
            case vmcode::BOS:
                if (sp == 0)
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::EOS:
                if (sp >= str.size ())
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::BOL:
                if (sp == 0 || str[sp - 1] == L'\n')
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::EOL:
                if (sp >= str.size ())
                    run->at (th--).ip = ip + 1;
                else if (str[sp] == L'\n')
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::WORDB:
                if (iswordboundary (str, sp))
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::NWORDB:
                if (! iswordboundary (str, sp))
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::SAVE:
                run->at (th).capture = ptrvec_copy_set (cap, a0, sp, -1);
                run->at (th--).ip = ip + 1;
                break;
            case vmcode::JMP:
                run->at (th--).ip = a0 + ip + 1;
                break;
            case vmcode::SPLIT:
                runthread (run, th + 1, a1 + ip + 1, sp, cap, cnt);
                run->at (th--).ip = a0 + ip + 1;
                break;
            case vmcode::RESET:
                run->at (th).counter = ptrvec_copy_set (cnt, prog[ip].reg, 0, 0);
                run->at (th--).ip = ip + 1;
                break;
            case vmcode::ISPLIT:
                {
                    int reg = prog[ip].reg;
                    int n1 = prog[ip].n1;
                    int n2 = prog[ip].n2;
                    int n = (*cnt)[reg] + 1;
                    run->at (th).counter = cnt = ptrvec_copy_set (cnt, reg, n, 0);
                    if (n <= n1)
                        run->at (th--).ip = ip + 1;
                    else if (n2 == -1 || n <= n2) {
                        runthread (run, th + 1, a1 + ip + 1, sp, cap, cnt);
                        run->at (th--).ip = a0 + ip + 1;
                    }
                }
                break;
            default:
                break;
            }
        }
        std::swap (run, rdy);
        rdy->clear ();
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
