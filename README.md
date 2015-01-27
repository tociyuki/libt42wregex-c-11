NAME
----

t42::wregex - plain regular expression library for C++11 std::wstring

SYNOPSYS
--------

    #include <vector>
    #include <string>
    #include <iostream>
    #include <locale>
    #include "t42wregex.hpp"

    int main ()
    {
        std::locale::global (std::locale (""));
        std::wcout.imbue (std::locale (""));

        t42::wregex re (L"a(.*)c");     // pattern
        std::wstring str (L"abcdcecf"); // target string
        std::vector<std::wstring::size_type> cap; // capture position list
        int rc = re.exec (str, cap, 0); // match from &str[0] with re
        if (rc != std::wstring::npos) { // match &str[0] to &str[rc - 1]
            // $0 entire matched slice
            std::wstring m0(str.begin () + cap[0], str.begin () + cap[1]);
            std::wcout << "m0 " << m0 << std::endl;
            // $1 first captured group
            if (cap[2] != std::wstring::npos && cap[3] != std::wstring::npos) {
                std::wstring m1(str.begin () + cap[2], str.begin () + cap[3]);
                std::wcout << "m1 " << m1 << std::endl;
            }
        }
        else {
            std::wcout << "not ok" << std::endl;
        }
        return EXIT_SUCCESS;
    }

For examples, to build and run this:

    $ clang++ -std=c++11 -c t42wrecomp.cpp
    $ clang++ -std=c++11 -c t42wreexec.cpp
    $ ar r libt42wregex.a t42wrecomp.o t42wreexec.o
    $ clang++ -std=c++11 -L./ -o example main.cpp -lt42wregex
    $ ./example

SYNTAX
------

Here is the t42::wregex's definition in Parsing Expression Grammar.

    regex <- cat '|' regex      # alternative
           / cat

    cat   <- term cat           # sequence of terms
           /

    term  <- factor '?'         # zero or one greedy
           / factor '*'         # zero or more greedy
           / factor '+'         # one or more greedy
           / factor '??'        # zero or one non-greedy
           / factor '*?'        # zero or more non-greedy
           / factor '+?'        # one or more non-greedy
           / factor '{' n '}'   # just n times
           / factor '{' n ',' m '}'  # n to m times greedy
           / factor '{' n ',}'  # more than or n times greedy
           / factor '{' n '}?'  # just n times
           / factor '{' n ',' m '}?'  # n to m times non-greedy
           / factor '{' n ',}?' # more than or n times non-greedy
           / factor

    factor <- '(' regex ')'     # capture group
           / '(?:' regex ')'    # uncaptured subexpression
                                ## lookahead (?=..) (?!..) are not implemented
                                ## option controls (?imsx:..) are not implemented
           / '.'                # any character includings with '\n'
           / '[' cclass ']'     # character class
           / '[^' cclass ']'    # complement character class
           / '^'                # begining of line
           / '$'                # end of line
           / '\\A'              # begining of string
           / '\\z'              # end of string
           / '\\b'              # at word boundary
           / '\\B'              # not at word boundary
           / '\\' ([1-7] ![0-7] / [8-9])    # back reference
           / char               # a character itself or an escaped character
                                ## \d \D \w \W \s \S are not implemented

    cclass <- char '-' char cclass  # range of characters
            / char cclass           # a character
                                    ## \d \D \w \W \s \S are not implemented
                                    ## [:XXX:] are not implemented

    char   <- '\\t'             # horizontal tab (\x09)
            / '\\n'             # new line       (\x0a)
            / '\\r'             # carrige return (\x0d)
            / '\\f'             # form feed      (\x0c)
            / '\\a'             # alarm bell     (\x07)
            / '\\e'             # escape         (\x1b)
            / '\\' [0-7]([0-7][0-7]?)?      # byte character by octal digits
            / '\\x' [0-9a-fA-F][0-9a-fA-F]  # byte character by hex digits
            / '\\x{' [0-9a-fA-F]+ '}'       # code point by hex digits
            / '\\' .            # wchar_t without control
            / .                 # wchar_t includes spaces

SEE ALSO
--------

 1. [Regular Expression Matching: the Virtual Machine Approach](http://swtch.com/~rsc/regexp/regexp2.html "Regular Expression Matching: the Virtual Machine Approach")

LISCENSE
--------

License: The BSD 3-Clause

Copyright (c) 2015, MIZUTANI Tociyuki
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

