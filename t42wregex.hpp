#ifndef T42WREGEX_H
#define T42WREGEX_H

#include <vector>
#include <string>
#include <memory>

namespace t42 {
namespace wpike {

struct vmspan {
    enum { STR, RANGE } type;
    std::wstring str;
    vmspan (std::wstring a) : type (STR), str (a) { }
    vmspan (wchar_t a, wchar_t b) : type (RANGE), str ()
    {
        str.push_back (a);
        str.push_back (b);
    }
    bool member (wchar_t const c);
};

struct vmcode {
    enum operation {
        MATCH, SAVE, CHAR, ANY, CCLASS, NCCLASS,
        BOL, EOL, BOS, EOS, WORDB, NWORDB, JMP, SPLIT
    };
    operation opcode;
    int addr0;
    int addr1;
    wchar_t ch;
    std::shared_ptr<std::vector<vmspan>> span;
    vmcode (operation a, int b, int c)
        : opcode (a), addr0 (b), addr1 (c), ch (), span () { }
    vmcode (operation a, wchar_t b)
        : opcode (a), addr0 (), addr1 (), ch (b), span () { }
    vmcode (operation a, std::shared_ptr<std::vector<vmspan>>& b)
        : opcode (a), addr0 (), addr1 (), ch (), span (b) { }
    vmcode (operation a)
        : opcode (a), addr0 (), addr1 (), ch (), span () { }
};

}//namespace wpike

class regex_error {};

class wregex {
public:
    enum {notmatch = -1};
    wregex (std::wstring s);
    int exec (std::wstring str, std::vector<int>& capture,
        std::wstring::size_type const pos);
private:
    std::vector<wpike::vmcode> prog;
};

}//namespace t42
#endif
