#include <vector>
#include <string>
#include <iostream>
#include <locale>
#include <utility>
#include "t42wregex.hpp"
#include "wtaptests.hpp"

std::wstring esc (std::wstring const& s)
{
    std::wstring t;
    for (auto c : s) {
        if (0x20 <= c && c < 0x7f)
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
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 7, L"qr/a(.*)c/ =~ \"abcdcec\"_\"f\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abcdcec", L"$0 == \"abcdcec\"");
    ts.ok (m1 == L"bcdce", L"$1 == \"bcdce\"");
}

void test2 (test::simple& ts)
{
    t42::wregex re (L"a(.*?)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 3, L"qr/a(.*?)c/ =~ \"abc\"_\"dcecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");
}

void test3 (test::simple& ts)
{
    t42::wregex re (L"a(.+)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 7, L"qr/a(.+)c/ =~ \"abcdcec\"_\"f\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abcdcec", L"$0 == \"abcdcec\"");
    ts.ok (m1 == L"bcdce", L"$1 == \"bcdce\"");
}

void test4 (test::simple& ts)
{
    t42::wregex re (L"a(.+?)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 3, L"qr/a(.+?)c/ =~ \"abc\"_\"dcecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");
}

void test5 (test::simple& ts)
{
    t42::wregex re (L"a(.*)c");
    std::wstring s (L"acdxexf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 2, L"qr/a(.*)c/ =~ \"ab\"_\"cdcecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"ac", L"$0 == \"ac\"");
    ts.ok (m1 == L"", L"$1 == \"\"");
}

void test6 (test::simple& ts)
{
    t42::wregex re (L"a(.+)c");
    std::wstring s (L"acdxexf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == std::wstring::npos, L"qr/a(.+)c/ !~ \"acdxexf\"");
}

void test7 (test::simple& ts)
{
    t42::wregex re (L"a(.?)c");
    std::wstring s (L"acdxexf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 2, L"qr/a(.?)c/ =~ \"ab\"_\"cdcecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"ac", L"$0 == \"ac\"");
    ts.ok (m1 == L"", L"$1 == \"\"");
}

void test8 (test::simple& ts)
{
    t42::wregex re (L"\\Aa(b)c\\z");
    std::wstring s (L"abc");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 3, L"qr/\\Aa(b)c\\z/ =~ \"abc\"_");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");

    std::wstring s1 (L"abc def");
    std::wstring::size_type rc1 = re.exec (s1, m, 0);
    ts.ok (rc1 == std::wstring::npos, L"qr/\\Aa(b)c\\z/ !~ \"abc def\"");
}

void test9 (test::simple& ts)
{
    t42::wregex re (L".*^a(b)c$");
    std::wstring s (L"a\nabc\nd\n");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 5, L"qr/.*^a(b)c$/ =~ \"a\\nabc\"_\"\\nd\\n\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"a\nabc", L"$0 == \"a\\nabc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");
}

void test10b (test::simple& ts)
{
    t42::wregex re (L".\\b.");
    std::wstring s1 (L"abc");
    t42::wregex::capture_list m;
    std::wstring::size_type rc1 = re.exec (s1, m, 0);
    ts.ok (rc1 == std::wstring::npos, L"qr/.\\b./ !~ \"abc\"");

    std::wstring s2 (L" abc");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == 2, L"qr/.\\b./ =~ \" a\"_\"bc\"");

    std::wstring s3 (L"a bc");
    std::wstring::size_type rc3 = re.exec (s3, m, 0);
    ts.ok (rc3 == 2, L"qr/.\\b./ =~ \"a \"_\"bc\"");
}

void test10 (test::simple& ts)
{
    t42::wregex re (L".*\\b(abc\\B..)");
    std::wstring s (L"Aabcde abc abcfghi");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 16, L"qr/.*\\b(abc\\B..)/ =~ \"Aabcde abc abcfg\"_\"hi\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"Aabcde abc abcfg", L"$0 == \"Aabcde abc abcfg\"");
    ts.ok (m1 == L"abcfg", L"$1 == \"abcfg\"");
}

void test11g (test::simple& ts)
{
    t42::wregex re (L".*([0-9]+(?:[.][0-9]*)?)");
    std::wstring s (L"number 3.1415 is the pi");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 13, L"qr/.*([0-9]+(?:[.][0-9]*)?)/ =~ \"number 3.1415\"_\" is the pi\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"number 3.1415", L"$0 == \"number 3.1415\"");
    ts.ok (m1 == L"5", L"$1 == \"5\"");
}

void test11 (test::simple& ts)
{
    t42::wregex re (L".*?([0-9]+(?:[.][0-9]*)?)");
    std::wstring s (L"number 3.1415 is the pi");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 13, L"qr/.*?([0-9]+(?:[.][0-9]*)?)/ =~ \"number 3.1415\"_\" is the pi\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"number 3.1415", L"$0 == \"number 3.1415\"");
    ts.ok (m1 == L"3.1415", L"$1 == \"3.1415\"");
}

void test12 (test::simple& ts)
{
    t42::wregex re (L"[^0-9]*([^.]+(?:[.][0-9]*)?)");
    std::wstring s (L"number 3.1415 is the pi");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 13, L"qr/[^0-9]*([^.]+(?:[.][0-9]*)?)/ =~ \"number 3.1415\"_\" is the pi\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"number 3.1415", L"$0 == \"number 3.1415\"");
    ts.ok (m1 == L"3.1415", L"$1 == \"3.1415\"");
}

void test13 (test::simple& ts)
{
    t42::wregex re (L"a(.{2,8})c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 7, L"qr/a(.{2,8})c/ =~ \"abcdcec\"_\"f\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abcdcec", L"$0 == \"abcdcec\"");
    ts.ok (m1 == L"bcdce", L"$1 == \"bcdce\"");

    std::wstring s2 (L"a1cdxexf");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/a(.{2,8})c/ !~ \"a1cdxexf\"");

    std::wstring s3 (L"a123456789c");
    std::wstring::size_type rc3 = re.exec (s3, m, 0);
    ts.ok (rc3 == std::wstring::npos, L"qr/a(.{2,8})c/ !~ \"a123456789c\"");
}

void test14 (test::simple& ts)
{
    t42::wregex re (L"a(.{2,8}?)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 5, L"qr/a(.{2,8}?)c/ =~ \"abcdc\"_\"ecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abcdc", L"$0 == \"abcdc\"");
    ts.ok (m1 == L"bcd", L"$1 == \"bcd\"");

    std::wstring s2 (L"a1cdxexf");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/a(.{2,8}?)c/ !~ \"a1cdxexf\"");

    std::wstring s3 (L"a123456789c");
    std::wstring::size_type rc3 = re.exec (s3, m, 0);
    ts.ok (rc3 == std::wstring::npos, L"qr/a(.{2,8}?)c/ !~ \"a123456789c\"");
}

void test13a (test::simple& ts)
{
    t42::wregex re (L"a(.{2})c");
    std::wstring s (L"acdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 4, L"qr/a(.{2})c/ =~ \"acdc\"_\"ecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"acdc", L"$0 == \"acdc\"");
    ts.ok (m1 == L"cd", L"$1 == \"cd\"");
}

void test13b (test::simple& ts)
{
    t42::wregex re (L"a(.{2}?)c");
    std::wstring s (L"acdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 4, L"qr/a(.{2})c/ =~ \"acdc\"_\"ecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"acdc", L"$0 == \"acdc\"");
    ts.ok (m1 == L"cd", L"$1 == \"cd\"");
}

void test15 (test::simple& ts)
{
    t42::wregex re (L"(.*?)\\b((?:[A-Z][a-z]{1,16}){2,4})\\b");
    std::wstring s (L"wiki has the FrontPage.");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 22, L"qr/(.*?)\\b((?:[A-Z][a-z]{1,16}){2,4})\\b/ =~ \"wiki has the FrontPage\"_\".\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    std::wstring m2(s.begin () + m[4], s.begin () + m[5]);
    ts.ok (m0 == L"wiki has the FrontPage", L"$0 == \"wiki has the FrontPage\"");
    ts.ok (m1 == L"wiki has the ", L"$1 == \"wiki has the \"");
    ts.ok (m2 == L"FrontPage", L"$2 == \"FrontPage\"");

    std::wstring s2 (L"wiki has the Frontpage.");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/(.*?)\\b((?:[A-Z][a-z]{1,16}){2,4})\\b/ !~ \"wiki has the Frontpage.\"");
}

void test16 (test::simple& ts)
{
    t42::wregex re (L"a(.{2,})c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 7, L"qr/a(.{2,})c/ =~ \"abcdcec\"_\"f\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abcdcec", L"$0 == \"abcdcec\"");
    ts.ok (m1 == L"bcdce", L"$1 == \"bcdce\"");

    std::wstring s2 (L"a1cdxexf");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/a(.{2,})c/ !~ \"a1cdxexf\"");

    std::wstring s3 (L"a123456789c");
    std::wstring::size_type rc3 = re.exec (s3, m, 0);
    ts.ok (rc3 == 11, L"qr/a(.{2,})c/ =~ \"a123456789c\"_");
}

void test17 (test::simple& ts)
{
    t42::wregex re (L"a(.{2,}?)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 5, L"qr/a(.{2,}?)c/ =~ \"abcdc\"_\"ecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abcdc", L"$0 == \"abcdc\"");
    ts.ok (m1 == L"bcd", L"$1 == \"bcd\"");

    std::wstring s2 (L"a1cdxexf");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/a(.{2,}?)c/ !~ \"a1cdxexf\"");

    std::wstring s3 (L"a123456789c");
    std::wstring::size_type rc3 = re.exec (s3, m, 0);
    ts.ok (rc3 == 11, L"qr/a(.{2,}?)c/ !~ \"a123456789c\"_");
}

void test18 (test::simple& ts)
{
    t42::wregex re (L"a(.|)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 3, L"qr/a(.|)c/ =~ \"abc\"_\"dcecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");

    std::wstring s2 (L"acdcecf");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == 2, L"qr/a(.|)c/ =~ \"ac\"_\"dcecf\"");
    m0.assign (s2.begin () + m[0], s2.begin () + m[1]);
    m1.assign (s2.begin () + m[2], s2.begin () + m[3]);
    ts.ok (m0 == L"ac", L"$0 == \"ac\"");
    ts.ok (m1 == L"", L"$1 == \"\"");
}

void test19 (test::simple& ts)
{
    t42::wregex re (L"a(|.)c");
    std::wstring s (L"abcdcecf");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 3, L"qr/a(|.)c/ =~ \"abc\"_\"dcecf\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abc", L"$0 == \"abc\"");
    ts.ok (m1 == L"b", L"$1 == \"b\"");

    std::wstring s2 (L"acdcecf");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == 2, L"qr/a(|.)c/ =~ \"ac\"_\"dcecf\"");
    m0.assign (s2.begin () + m[0], s2.begin () + m[1]);
    m1.assign (s2.begin () + m[2], s2.begin () + m[3]);
    ts.ok (m0 == L"ac", L"$0 == \"ac\"");
    ts.ok (m1 == L"", L"$1 == \"\"");
}

void test20 (test::simple& ts)
{
    t42::wregex re (L"(.*?)(<([a-z]+)>.*?</\\3>)");
    std::wstring s (L"a b <strong>c <em>d</em> f</strong> g");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 35, L"qr/(.*?)(<([a-z]+)>.*?</\\3>)/ =~ \"a b <strong>c <em>d</em> f</strong>\"_\" g\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    std::wstring m2(s.begin () + m[4], s.begin () + m[5]);
    ts.ok (m0 == L"a b <strong>c <em>d</em> f</strong>", L"$0 == \"a b <strong>c <em>d</em> f</strong>\"");
    ts.ok (m1 == L"a b ", L"$1 == \"a b \"");
    ts.ok (m2 == L"<strong>c <em>d</em> f</strong>", L"$0 == \"<strong>c <em>d</em> f</strong>\"");
}

void test21 (test::simple& ts)
{
    t42::wregex re (L"[a-z]+(?=([.!?]))(.:)");
    std::wstring s (L"lookahead!: works");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 11, L"qr/[a-z]+(?=([.!?]))(.:)/ =~ \"lookahead!:\"_\" works\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    std::wstring m2(s.begin () + m[4], s.begin () + m[5]);
    ts.ok (m0 == L"lookahead!:", L"$0 == \"lookahead!:\"");
    ts.ok (m1 == L"!", L"$1 == \"!\"");
    ts.ok (m2 == L"!:", L"$0 == \"!:\"");

    std::wstring s2 (L"lookahead,: works");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/[a-z]+(?=([.!?]))(.:)/ !~ \"lookahead,: works\"");
}

void test22 (test::simple& ts)
{
    t42::wregex re (L"abc(?!efg)(.{3,});");
    std::wstring s (L"abc345; negative lookahead.");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 7, L"qr/abc(?!efg)(.{3,});/ =~ \"abc345;\"_\" negative lookahead.\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m0 == L"abc345;", L"$0 == \"abc345;\"");
    ts.ok (m1 == L"345", L"$1 == \"345\"");

    std::wstring s2 (L"abcefg; negative lookahead.");
    std::wstring::size_type rc2 = re.exec (s2, m, 0);
    ts.ok (rc2 == std::wstring::npos, L"qr/abc(?!efg)(.{3,});/ !~ \"abcefg; negative lookahead.\"");
}

void test23g (test::simple& ts)
{
    t42::wregex re (L"one(self)?(selfsufficient)?");
    std::wstring s (L"oneselfsufficient");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 7, L"qr/one(self)?(selfsufficient)?/ =~ \"oneself\"_\"sufficient\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    ts.ok (m0 == L"oneself", L"$0 == \"oneself\"");
    //ts.diag (std::wstring (L"m.size ()==") + std::to_wstring (m.size ()));
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m1 == L"self", L"$1 == \"self\"");
}

void test23n (test::simple& ts)
{
    t42::wregex re (L"one(self)?\?(selfsufficient)?"); // avoid trigraph
    std::wstring s (L"oneselfsufficient");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 17, L"qr/one(self)?(selfsufficient)?/ =~ \"oneselfsufficient\"_");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    ts.ok (m0 == L"oneselfsufficient", L"$0 == \"oneselfsufficient\"");
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m1 == L"", L"$1 == \"\"");
    std::wstring m2(s.begin () + m[4], s.begin () + m[5]);
    ts.ok (m2 == L"selfsufficient", L"$2 == \"selfsufficient\"");
}

void test24 (test::simple& ts)
{
// https://github.com/shirok/Gauche/blob/master/test/regexp.scm
    t42::wregex::capture_list m;
    t42::wregex re1 (L".*?((?<=a)b)");
    std::wstring s1 (L"b");
    ts.ok (re1.exec (s1, m, 0) == std::wstring::npos, L"/.*?((?<=a)b)/ !~ \"b\"");
    std::wstring s2 (L"ab");
    ts.ok (re1.exec (s2, m, 0) == 2, L"/.*?((?<=a)b)/ =~ \"ab\"");
    ts.ok (s2.substr (m[2], m[3] - m[2]) == L"b", L"$1 \"b\"");
    t42::wregex re3 (L".*?((?<=a+)b)");
    std::wstring s3 (L"aab");
    ts.ok (re3.exec (s3, m, 0) == 3, L"/.*?((?<=a+)b)/ =~ \"aab\"");
    ts.ok (s3.substr (m[2], m[3] - m[2]) == L"b", L"$1 \"b\"");
    t42::wregex re4 (L".*?((?<=x[yz])b)");
    std::wstring s4 (L"xzb");
    ts.ok (re4.exec (s4, m, 0) == 3, L"/.*?((?<=x[yz])b)/ =~ \"xzb\"");
    t42::wregex re5 (L".*?((?<=zyx)b)");
    std::wstring s5 (L"xyzb");
    ts.ok (re5.exec (s5, m, 0), L"/.*?((?<=zyx)b)/ !~ \"xyzb\"");
    t42::wregex re6 (L".*?((?<=[ab]+)c)");
    std::wstring s6 (L"abc");
    ts.ok (re6.exec (s6, m, 0) == 3, L"/.*?((?<=[ab]+)c)/ =~ \"abc\"");
    t42::wregex re7 (L".*?((?<!<[^>]*)foo)");
    std::wstring s7 (L"<foo>");
    ts.ok (re7.exec (s7, m, 0) == std::wstring::npos, L"/.*?((?<!<[^>]*)foo)/ =~ \"<foo>\"");
    std::wstring s8 (L"<bar>foo");
    ts.ok (re7.exec (s8, m, 0) == 8, L"/.*?((?<!<[^>]*)foo)/ =~ \"<bar>foo\"");
    ts.ok (s8.substr (m[2], m[3] - m[2]) == L"foo", L"$1 \"foo\"");
    t42::wregex re9 (L".*?((?<=^a)b)");
    std::wstring s9 (L"ab");
    ts.ok (re9.exec (s9, m, 0) == 2, L"/.*?((?<=^a)b)/ =~ \"ab\"");
    std::wstring s10 (L",,,\nab");
    ts.ok (re9.exec (s10, m, 0) == 6, L"/.*?((?<=^a)b)/ =~ \",,,\\nab\"");
    t42::wregex re11 (L".*?((?<=^)b)");
    std::wstring s11 (L"ab");
    ts.ok (re11.exec (s11, m, 0) == std::wstring::npos, L"/.*?((?<=^)b)/ !~ \"ab\"");
    std::wstring s12 (L"b");
    ts.ok (re11.exec (s12, m, 0) == 1, L"/.*?((?<=^)b)/ =~ \"b\"");
    t42::wregex re13 (L".(?<=^)b");
    std::wstring s13 (L"a^b");
    ts.ok (re13.exec (s13, m, 0) == std::wstring::npos, L"/.(?<=^)b/ !~ \"a^b\"");
    t42::wregex re14 (L".*?((?<=^a)$)");
    std::wstring s14 (L"a");
    ts.ok (re14.exec (s14, m, 0) == 1, L"/(?<=^a)$/ =~ \"a\"");
    ts.ok (s14.substr (m[2], m[3] - m[2]) == L"", L"$1 \"\"");
    t42::wregex re15 (L".*?((?<=(a))b)");
    std::wstring s15 (L"ab");
    ts.ok (re15.exec (s15, m, 0) == 2, L"/.*?((?<=(a))b)/ =~ \"ab\"");
    ts.ok (s15.substr (m[2], m[3] - m[2]) == L"b", L"$1 \"b\"");
    ts.ok (s15.substr (m[4], m[5] - m[4]) == L"a", L"$2 \"a\"");
    t42::wregex re16 (L".*?((?<=(a)(b))c)");
    std::wstring s16 (L"abc");
    ts.ok (re16.exec (s16, m, 0) == 3, L"/.*?((?<=(a)(b))c)/ =~ \"abc\"");
    ts.ok (s16.substr (m[2], m[3] - m[2]) == L"c", L"$1 \"c\"");
    ts.ok (s16.substr (m[4], m[5] - m[4]) == L"a", L"$2 \"a\"");
    ts.ok (s16.substr (m[6], m[7] - m[6]) == L"b", L"$3 \"b\"");
    t42::wregex re17 (L".*?((?<=(a)|(b))c)");
    std::wstring s17 (L"bc");
    ts.ok (re17.exec (s17, m, 0) == 2, L"/.*?((?<=(a)|(b))c)/ =~ \"bc\"");
    ts.ok (s17.substr (m[2], m[3] - m[2]) == L"c", L"$1 \"c\"");
    ts.ok (m[4] >= s17.size () && m[5] >= s17.size (), L"$2 nil");
    ts.ok (s17.substr (m[6], m[7] - m[6]) == L"b", L"$3 \"b\"");
    t42::wregex re18 (L".*?((?<=(?<!foo)bar)baz)");
    std::wstring s18 (L"abarbaz");
    ts.ok (re18.exec (s18, m, 0) == 7, L"/.*?((?<=(?<!foo)bar)baz)/ =~ \"abarbaz\"");
    std::wstring s19 (L"foobarbaz");
    ts.ok (re18.exec (s19, m, 0) == std::wstring::npos, L"/.*?((?<=(?<!foo)bar)baz)/ !~ \"foobarbaz\"");
    t42::wregex re20 (L".*?((?<=[0-9][0-9][0-9])(?<!999)foo)");
    std::wstring s20 (L"865foo");
    ts.ok (re20.exec (s20, m, 0) == 6, L"/.*?((?<=[0-9][0-9][0-9])(?<!999)foo)/ =~ \"865foo\"");
    std::wstring s21 (L"999foo");
    ts.ok (re20.exec (s21, m, 0) == std::wstring::npos, L"/.*?((?<=[0-9][0-9][0-9])(?<!999)foo)/ !~ \"999foo\"");
    t42::wregex re22 (L".*?(abc)...(?<=\\1)");
    std::wstring s22 (L"abcabc");
    ts.ok (re22.exec (s22, m, 0) == 6, L"/.*?(abc)...(?<=\\1)/ =~ \"abcabc\"");
}

void test25 (test::simple& ts)
{
    t42::wregex re (L".*?((?<=_(?=[0-9][0-9]~))[0-9]+)");
    std::wstring s (L"_12~");
    t42::wregex::capture_list m;
    std::wstring::size_type rc = re.exec (s, m, 0);
    ts.ok (rc == 3, L"qr/.*?((?<=_(?=[0-9][0-9]~))[0-9]+)/ =~ \"_12\"_\"~\"");
    std::wstring m0(s.begin () + m[0], s.begin () + m[1]);
    ts.ok (m0 == L"_12", L"$0 == \"_12\"");
    std::wstring m1(s.begin () + m[2], s.begin () + m[3]);
    ts.ok (m1 == L"12", L"$1 == \"12\"");
}

void test26 (test::simple& ts)
{
    t42::wregex::capture_list m;
    std::wstring s (L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_ \t\r\n\v\f!@#$%^&*()-+=|~");
    t42::wregex re1 (L".*?(\\d+)");
    std::wstring::size_type rc1 = re1.exec (s, m, 0);
    ts.ok (rc1 == 10, L"qr/.*?(\\d+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"0123456789", L"$1 \"0123456789\"");
    t42::wregex re2 (L".*?(\\D+)");
    std::wstring::size_type rc2 = re2.exec (s, m, 0);
    ts.ok (rc2 == 84, L"qr/.*?(\\D+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_ \t\r\n\v\f!@#$%^&*()-+=|~", L"$1 \"aA..\"");
    t42::wregex re3 (L".*?(\\w+)");
    std::wstring::size_type rc3 = re3.exec (s, m, 0);
    ts.ok (rc3 == 63, L"qr/.*?(\\w+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_", L"$1 \"0123456789Aa..\"");
    t42::wregex re4 (L".*?(\\W+)");
    std::wstring::size_type rc4 = re4.exec (s, m, 0);
    ts.ok (rc4 == 84, L"qr/.*?(\\W+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L" \t\r\n\v\f!@#$%^&*()-+=|~", L"$1 \" \\t..\"");
    std::wstring s5 (L" \t\r\n\v\f0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_!@#$%^&*()-+=|~");
    t42::wregex re5 (L".*?(\\s+)");
    std::wstring::size_type rc5 = re5.exec (s5, m, 0);
    ts.ok (rc5 == 6, L"qr/.*?(\\s+)/ =~ \" \\t..\"");
    ts.ok (s5.substr (m[2], m[3] - m[2]) == L" \t\r\n\v\f", L"$1 \" \\t..\"");
    t42::wregex re6 (L".*?(\\S+)");
    std::wstring::size_type rc6 = re6.exec (s5, m, 0);
    ts.ok (rc6 == 84, L"qr/.*?(\\S+)/ =~ \" \\t..\"");
    ts.ok (s5.substr (m[2], m[3] - m[2]) == L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_!@#$%^&*()-+=|~", L"$1 \"0123456789aA..\"");
}

void test27 (test::simple& ts)
{
    t42::wregex::capture_list m;
    std::wstring s (L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_ \t\r\n\v\f!@#$%^&*()-+=|~");
    t42::wregex re1 (L".*?([\\d]+)");
    std::wstring::size_type rc1 = re1.exec (s, m, 0);
    ts.ok (rc1 == 10, L"qr/.*?([\\d]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"0123456789", L"$1 \"0123456789\"");
    t42::wregex re2 (L".*?([\\D]+)");
    std::wstring::size_type rc2 = re2.exec (s, m, 0);
    ts.ok (rc2 == 84, L"qr/.*?([\\D]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_ \t\r\n\v\f!@#$%^&*()-+=|~", L"$1 \"aA..\"");
    t42::wregex re3 (L".*?([\\w]+)");
    std::wstring::size_type rc3 = re3.exec (s, m, 0);
    ts.ok (rc3 == 63, L"qr/.*?([\\w]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_", L"$1 \"0123456789Aa..\"");
    t42::wregex re4 (L".*?([\\W]+)");
    std::wstring::size_type rc4 = re4.exec (s, m, 0);
    ts.ok (rc4 == 84, L"qr/.*?([\\W]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L" \t\r\n\v\f!@#$%^&*()-+=|~", L"$1 \" \\t..\"");
    t42::wregex re5 (L".*?([^\\W\\d]+)");
    std::wstring::size_type rc5 = re5.exec (s, m, 0);
    ts.ok (rc5 == 63, L"qr/.*?([^\\W\\d]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_", L"$1 \"aAbB..zZ_\"");
}

void test28 (test::simple& ts)
{
    t42::wregex::capture_list m;
    std::wstring s (L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_ \t\r\n\v\f!@#$%^&*()-+=|~");
    t42::wregex re1 (L".*?([[:digit:]]+)");
    std::wstring::size_type rc1 = re1.exec (s, m, 0);
    ts.ok (rc1 == 10, L"qr/.*?([[:digit:]]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"0123456789", L"$1 \"0123456789\"");
    t42::wregex re2 (L".*?([[:^digit:]]+)");
    std::wstring::size_type rc2 = re2.exec (s, m, 0);
    ts.ok (rc2 == 84, L"qr/.*?([[:^digit:]]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_ \t\r\n\v\f!@#$%^&*()-+=|~", L"$1 \"aA..\"");
    t42::wregex re3 (L".*?([[:alnum:]_]+)");
    std::wstring::size_type rc3 = re3.exec (s, m, 0);
    ts.ok (rc3 == 63, L"qr/.*?([[:alnum:]_]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"0123456789aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_", L"$1 \"0123456789Aa..\"");
    t42::wregex re5 (L".*?([[:alpha:]_]+)");
    std::wstring::size_type rc5 = re5.exec (s, m, 0);
    ts.ok (rc5 == 63, L"qr/.*?([[:alpha:]_]+)/ =~ \"0123456789aA..\"");
    ts.ok (s.substr (m[2], m[3] - m[2]) == L"aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ_", L"$1 \"aAbB..zZ_\"");
}

void test29 (test::simple& ts)
{
    t42::wregex::capture_list m;
    t42::wregex re1 (L"FOO", t42::wregex::icase);
    std::wstring s1 (L"foo");
    std::wstring::size_type rc1 = re1.exec (s1, m, 0);
    ts.ok (rc1 == 3, L"qr/FOO/i =~ \"foo\"_");

    t42::wregex re2 (L"[F][O][O]", t42::wregex::icase);
    std::wstring::size_type rc2 = re2.exec (s1, m, 0);
    ts.ok (rc2 == 3, L"qr/[F][O][O]/i =~ \"foo\"_");

    t42::wregex re3 (L"[A-Z]+", t42::wregex::icase);
    std::wstring::size_type rc3 = re3.exec (s1, m, 0);
    ts.ok (rc3 == 3, L"qr/[A-Z]+/i =~ \"foo\"_");

    t42::wregex re4 (L"<([A-Z]+)>.*?</\\1>", t42::wregex::icase);
    std::wstring s4 (L"<EM>emphasis</em>");
    std::wstring::size_type rc4 = re4.exec (s4, m, 0);
    ts.ok (rc4 == 17, L"qr/<([A-Z]+)>.*?</\\1>/i =~ \"<EM>emphasis</em>\"_");
}

void test30 (test::simple& ts)
{
    t42::wregex::capture_list m;
    t42::wregex re1 (L"(?*<|>|[^<>])");
    std::wstring s1 (L"<a<b<>c<d>>e<f>g>!");
    std::wstring::size_type rc1 = re1.exec (s1, m, 0);
    ts.ok (rc1 == 17, L"qr/(?*<|>|[^<>])/ =~ \"<a<b<>c<d>>e<f>g>\"_\"!\"");

    //t42::wregex re2 (L"(?*/\\*|\\*/|(?!/\\*|\\*/).)");
    t42::wregex re2 (L"(?*/\\*|\\*/|[^/*]|/(?!\\*)|\\*(?!/))");
    std::wstring s2 (L"/*c * /*o/**/m*/m//* /e**/nt*/!");
    std::wstring::size_type rc2 = re2.exec (s2, m, 0);
    ts.ok (rc2 == 30, L"qr/(?*/\\*|\\*/|[^/*]|/(?!\\*)|\\*(?!/))/ =~ \"/*c * /*o/**/m*/m//* /e**/nt*/\"_\"!\"");
}

int main (int argc, char* argv[])
{
    std::locale::global (std::locale (""));
    std::wcout.imbue (std::locale (""));

    test::simple ts (177);

    test1 (ts);
    test2 (ts);
    test3 (ts);
    test4 (ts);
    test5 (ts);
    test6 (ts);
    test7 (ts);
    test8 (ts);
    test9 (ts);
    test10b (ts);
    test10 (ts);
    test11g (ts);
    test11 (ts);
    test12 (ts);
    test13 (ts);
    test13a (ts);
    test13b (ts);
    test14 (ts);
    test15 (ts);
    test16 (ts);
    test17 (ts);
    test18 (ts);
    test19 (ts);
    test20 (ts);
    test21 (ts);
    test22 (ts);
    test23g (ts);
    test23n (ts);
    test24 (ts);
    test25 (ts);
    test26 (ts);
    test27 (ts);
    test28 (ts);
    test29 (ts);
    test30 (ts);
    return ts.done_testing ();
}

