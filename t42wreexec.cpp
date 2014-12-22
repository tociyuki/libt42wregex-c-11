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
        if (c >= from && c <= last)
            return true;
    }
    return false;
}

struct vmthread {
    int ip;
    std::shared_ptr<std::vector<int>> capture;
    vmthread (int a, std::shared_ptr<std::vector<int>> const& b)
        : ip (a), capture (b) { }
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
        std::vector<vmthread>* const que, int const from, int const ip,
        std::shared_ptr<std::vector<int>>& capture);
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
    else if (! isword (str[sp - 1]) && isword (str[sp]))
        return true;
    else if (isword (str[sp - 1]) && ! isword (str[sp]))
        return true;
    else if (sp + 1 < str.size() && isword (str[sp]) && ! isword (str[sp + 1]))
        return true;
    else if (sp + 1 < str.size() && ! isword (str[sp]) && isword (str[sp + 1]))
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
    std::vector<vmthread>* const que, int const from, int const ip,
    std::shared_ptr<std::vector<int>>& capture)
{
    for (int i = from; i < que->size (); i++) {
        if (que->at (i).ip == ip) {
            que->at (i).capture = capture;
            return;
        }
    }
    vmthread th (ip, capture);
    que->push_back (th);
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
    runthread (run, 0, PROGSTART, cap0);
    rdy->clear ();
    for (int sp = pos; ; sp++) {
        for (int th = 0; th < run->size(); th++) {
            int ip = run->at (th).ip;
            auto cap = run->at (th).capture;
            if (vmcode::MATCH == prog[ip].opcode) {
                match = sp;
                capture.clear ();
                for (auto x : *cap)
                    capture.push_back (x);
                capture[1] = sp;
                break;
            }
            if (vmcode::SAVE == prog[ip].opcode) {
                auto newcap = std::make_shared<std::vector<int>> ();
                for (auto x : *cap)
                    newcap->push_back (x);
                int num = prog[ip].addr0;
                while (num >= newcap->size ())
                    newcap->push_back (-1);
                (*newcap)[num] = sp;
                run->at (th).capture = newcap;
                run->at (th--).ip = ip + 1;
                continue;
            }
            switch (prog[ip].opcode) {
            case vmcode::CHAR:
                if (sp < str.size () && str[sp] == prog[ip].ch)
                    runthread (rdy, 0, ip + 1, cap);
                break;
            case vmcode::ANY:
                if (sp < str.size ())
                    runthread (rdy, 0, ip + 1, cap);
                break;
            case vmcode::CCLASS:
                if (sp < str.size () && incclass (str[sp], prog[ip].span))
                    runthread (rdy, 0, ip + 1, cap);
                break;
            case vmcode::NCCLASS:
                if (sp < str.size () && incclass (str[sp], prog[ip].span))
                    runthread (rdy, 0, ip + 1, cap);
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
                    runthread (rdy, 0, ip + 1, cap);
                break;
            case vmcode::WORDB:
                if (iswordboundary (str, sp))
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::NWORDB:
                if (! iswordboundary (str, sp))
                    run->at (th--).ip = ip + 1;
                break;
            case vmcode::JMP:
                run->at (th--).ip = prog[ip].addr0;
                break;
            case vmcode::SPLIT:
                runthread (run, th + 1, prog[ip].addr1, cap);
                run->at (th--).ip = prog[ip].addr0;
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

