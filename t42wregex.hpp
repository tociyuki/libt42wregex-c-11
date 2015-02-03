#ifndef T42WREGEX_H
#define T42WREGEX_H

#include <vector>
#include <string>

namespace t42 {
namespace wpike {

enum operation {
    MATCH, CHAR, ANY, CCLASS, NCCLASS, BKREF, // non-epsilon
    BOL, EOL, BOS, EOS, WORDB, NWORDB, SAVE,  // epsilon
    JMP, SPLIT, LKAHEAD, NLKAHEAD, LKBEHIND, NLKBEHIND, RESET, REP // epsilon
};

struct instruction {
    operation opcode;
    std::wstring s;
    int x;
    int y;
    int r;
    instruction (operation const a) : opcode (a), s (), x (0), y (0), r (0) {}
    instruction (operation const a, std::wstring const &b)
        : opcode (a), s (b), x (0), y (0), r (0) {}
    instruction (operation const a, int const b, int const c, int const d)
        : opcode (a), s (), x (b), y (c), r (d) {}
};

typedef std::vector<instruction> program;
typedef std::vector<std::wstring::size_type> capture_list;

int c7toi (wchar_t c);

}//namespace wpike

class regex_error {};

class wregex {
public:
    typedef wpike::capture_list capture_list;
    wregex (std::wstring pat);
    std::wstring::size_type exec (std::wstring const s,
        capture_list& m, std::wstring::size_type const sp, char const* lc = "") const;
    wpike::program prog() { return e; }
private:
    wpike::program e;
};

}//namespace t42
#endif
