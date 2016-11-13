//
// CDAP connector tests
//
// Micheal Crotty <mcrotty AT tssg DOT org>
// Copyright (c) 2016, PRISTINE project, Waterford Institute of Technology. 
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <list>
#include <string>

#include "catch.h"
#include "../common/utils.h"

using namespace std;

/*
void parse_dif_names(std::list<std::string> & dif_names, const std::string& arg)
{
   const char c = ',';
   string::size_type s = 0;
   string::size_type e = arg.find(c);

   while (e != string::npos) {
      dif_names.push_back(arg.substr(s, e-s));
      s = ++e;
      e = arg.find(c, e);
   }
   // Grab the remaining
   if (e == string::npos)
     dif_names.push_back(arg.substr(s, arg.length()));
}
*/

TEST_CASE("Command line parsing", "[util]") {
  
  list<string> v;

  SECTION("Parse 1 DIF name") {
    v.clear();
    string one_name("one");  
    
    parse_dif_names(v, one_name);
    
    REQUIRE(!v.empty());
    REQUIRE(v.size() == 1);
    REQUIRE(*(v.begin()) == string("one"));
  }
  
  SECTION("Parse 2 DIF names") {
    v.clear();
    string two_names("one,two");  
    
    parse_dif_names(v, two_names);
    
    REQUIRE(!v.empty());
    REQUIRE(v.size() == 2);
    auto it = v.begin();
    REQUIRE(*it == string("one"));
    it++;
    REQUIRE(*it == string("two"));
  }
  
  SECTION("Parse 3 DIF names") {
    v.clear();
    string three_names("one,two,three");  
    
    parse_dif_names(v, three_names);
    
    REQUIRE(!v.empty());
    REQUIRE(v.size() == 3);
    auto it = v.begin();
    REQUIRE(*it == string("one"));
    it++;
    REQUIRE(*it == string("two"));
    it++;
    REQUIRE(*it == string("three"));
  }
  
  SECTION("Parse empty DIF name") {
    v.clear();
    string empty_name;  
    
    parse_dif_names(v, empty_name);
    
    REQUIRE(!v.empty());
    REQUIRE(v.size() == 1);
    REQUIRE(*(v.begin()) == string(""));
  }

}
