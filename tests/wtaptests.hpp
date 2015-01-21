#include <iostream>
#include <string>

namespace test {
class simple {
private:
    bool mno_plan, mskip, mtodo, mprt_nums;
    int muse_nums, mcur_test, mtest_skip, mtest_ok, mtest_notok, mtest_todo;

public:
    simple ()
        : mno_plan (true), mskip (false), mtodo (false), mprt_nums (false),
          muse_nums (0), mcur_test (0),
          mtest_skip (0), mtest_ok (0), mtest_notok (0), mtest_todo (0) { }

    simple (int n)
        : mno_plan (false), mskip (false), mtodo (false), mprt_nums (true),
          muse_nums (n), mcur_test (0),
          mtest_skip (0), mtest_ok (0), mtest_notok (0), mtest_todo (0) { }

    void skip () { mskip = true; }
    void todo () { mtodo = true; }

    bool ok (bool tst, std::wstring desc)
    {
        if (mprt_nums) {
            std::wcout << L"1.." << muse_nums << std::endl;
            mprt_nums = false;
        }
        ++mcur_test;
        if (mskip) {
            ++mtest_skip;
            std::wcout << L"ok " << mcur_test << L" # SKIP";
            if (! desc.empty ()) {
                std::wcout << L" " << desc;
            }
        }
        else if (tst) {
            ++mtest_ok;
            std::wcout << L"ok " << mcur_test;
            if (! desc.empty ()) {
                std::wcout << L" - " << desc;
            }
        }
        else {
            ++mtest_notok;
            std::wcout << L"not ok " << mcur_test;
            if (! desc.empty ()) {
                std::wcout << L" - " << desc;
            }
            if (mtodo) {
                std::wcout << L" # TODO";
            }
        }
        std::wcout << std::endl;
        mskip = false;
        mtodo = false;
        return tst;
    }

    void diag (std::wstring s)
    {
        std::wcout << L"# ";
        for (auto c : s) {
            std::wcout << c;
            if (c == L'\n') {
                std::wcout << L"# ";
            }
        }
        std::wcout << std::endl;
    }

    int done_testing ()
    {
        if (mno_plan) {
            muse_nums = mcur_test;
            mprt_nums = true;
        }
        if (mprt_nums) {
            std::wcout << L"1.." << mcur_test << std::endl;
            mprt_nums = false;
        }
        return muse_nums == mtest_ok + mtest_skip ? EXIT_SUCCESS : EXIT_FAILURE;
    }
};
} //namespace test
