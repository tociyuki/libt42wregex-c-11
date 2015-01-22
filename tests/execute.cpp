#include <vector>
#include <string>
#include <iostream>
#include <locale>
#include <utility>
#include "t42wregex.hpp"
#include "wtaptests.hpp"

/*
 * c++ -std=c++11 -I.. -c ../t42wrecomp.cpp
 * c++ -std=c++11 -I.. -c ../t42wreexec.cpp
 * c++ -std=c++11 -o execute -I. -I.. execute.cpp t42wrecomp.o  t42wreexec.o
 * ./execute
 */

std::wstring esc (std::wstring const& s)
{
    std::wstring t;
    for (auto c : s) {
        if (0x20 <= c && c < 0x7e)
            t.push_back (c);
        else if (L'\t' == c)
            t += L"\\t";
        else if (0x07 == c)
            t += L"\\a";
        else if (L'\f' == c)
            t += L"\\f";
        else if (L'\r' == c)
            t += L"\\r";
        else if (L'\n' == c)
            t += L"\\n";
        else if (0x1b == c)
            t += L"\\e";
        else if (0 <= c && c < 256) {
            unsigned int hi = (c >> 4) & 0x0f;
            unsigned int lo = c & 0x0f;
            t += L"\\x";
            t.push_back (hi < 10 ? hi + L'0' : hi + L'a' - 10);
            t.push_back (lo < 10 ? lo + L'0' : lo + L'a' - 10);
        }
    }
    return std::move (t);
}

std::wstring esc (wchar_t c)
{
    std::wstring s (1, c);
    return esc (s);
}

void test1 (test::simple& ts)
{
    t42::wregex re (L"a(.*)c");
    std::wstring str (L"abcdcecf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 7, L"qr/a(.*)c/ =~ \"abcdcec\"_\"f\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"abcdcec", L"$0 == \"abcdcec\"");
    ts.ok (m1 == L"bcdce", L"$1 == \"bcdce\"");
}

void test2 (test::simple& ts)
{
    t42::wregex re (L"a(.*?)c");
    std::wstring str (L"abcdcecf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 3, L"qr/a(.*?)c/ =~ \"abc\"_\"dcecf\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");
}

void test3 (test::simple& ts)
{
    t42::wregex re (L"a(.+)c");
    std::wstring str (L"abcdcecf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 7, L"qr/a(.+)c/ =~ \"abcdcec\"_\"f\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"abcdcec", L"$0 == \"abcdcec\"");
    ts.ok (m1 == L"bcdce", L"$1 == \"bcdce\"");
}

void test4 (test::simple& ts)
{
    t42::wregex re (L"a(.+?)c");
    std::wstring str (L"abcdcecf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 3, L"qr/a(.+?)c/ =~ \"abc\"_\"dcecf\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");
}

void test5 (test::simple& ts)
{
    t42::wregex re (L"a(.*)c");
    std::wstring str (L"acdxexf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 2, L"qr/a(.*)c/ =~ \"ab\"_\"cdcecf\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"ac", L"$0 == \"ac\"");
    ts.ok (m1 == L"", L"$1 == \"\"");
}

void test6 (test::simple& ts)
{
    t42::wregex re (L"a(.+)c");
    std::wstring str (L"acdxexf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc < 0, L"qr/a(.+)c/ !~ \"acdxexf\"");
}

void test7 (test::simple& ts)
{
    t42::wregex re (L"a(.?)c");
    std::wstring str (L"acdxexf");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 2, L"qr/a(.?)c/ =~ \"ab\"_\"cdcecf\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"ac", L"$0 == \"ac\"");
    ts.ok (m1 == L"", L"$1 == \"\"");
}

void test8 (test::simple& ts)
{
    t42::wregex re (L"\\Aa(b)c\\z");
    std::wstring str (L"abc");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 3, L"qr/\\Aa(b)c\\z/ =~ \"abc\"_");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");

    std::wstring str1 (L"abc def");
    int rc1 = re.exec (str1, cap, 0);
    ts.ok (rc1 < 0, L"qr/\\Aa(b)c\\z/ !~ \"abc def\"");
}

void test9 (test::simple& ts)
{
    t42::wregex re (L".*^a(b)c$");
    std::wstring str (L"a\nabc\nd\n");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 5, L"qr/.*^a(b)c$/ =~ \"a\\nabc\"_\"\\nd\\n\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"a\nabc", L"$0 == \"a\\nabc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");
}

void test10 (test::simple& ts)
{
    t42::wregex re (L".*\\b(abc\\B..)");
    std::wstring str (L"Aabcde abc abcfghi");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 16, L"qr/.*\\b(abc\\B..)/ =~ \"Aabcde abc abcfg\"_\"hi\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"Aabcde abc abcfg", L"$0 == \"Aabcde abc abcfg\"");
    ts.ok (m1 == L"abcfg", L"$1 == \"abcfg\"");
}

void test11 (test::simple& ts)
{
    t42::wregex re (L".*([0-9]+(?:[.][0-9]*)?)");
    std::wstring str (L"number 3.1415 is the pi");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 13, L"qr/.*([0-9]+(?:[.][0-9]*)?)/ =~ \"number 3.1415\"_\" is the pi\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"number 3.1415", L"$0 == \"number 3.1415\"");
    ts.ok (m1 == L"3.1415", L"$1 == \"3.1415\"");
}

void test12 (test::simple& ts)
{
    t42::wregex re (L"[^0-9]*([^.]+(?:[.][0-9]*)?)");
    std::wstring str (L"number 3.1415 is the pi");
    std::vector<int> cap;
    int rc = re.exec (str, cap, 0);
    ts.ok (rc == 13, L"qr/[^0-9]*([^.]+(?:[.][0-9]*)?)/ =~ \"number 3.1415\"_\" is the pi\"");
    std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
    std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
    ts.ok (m0 == L"number 3.1415", L"$0 == \"number 3.1415\"");
    ts.ok (m1 == L"3.1415", L"$1 == \"3.1415\"");
}

int main (int argc, char* argv[])
{
    std::locale::global (std::locale (""));
    std::wcout.imbue (std::locale (""));

    test::simple ts (35);
    test1 (ts);
    test2 (ts);
    test3 (ts);
    test4 (ts);
    test5 (ts);
    test6 (ts);
    test7 (ts);
    test8 (ts);
    test9 (ts);
    test10 (ts);
    test11 (ts);
    test12 (ts);
    return ts.done_testing ();
}

