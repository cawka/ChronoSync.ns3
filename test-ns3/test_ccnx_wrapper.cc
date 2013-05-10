/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Chaoyi Bian <bcy@pku.edu.cn>
 *	   Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/output_test_stream.hpp> 
using boost::test_tools::output_test_stream;

#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ndnSIM-module.h>

#include "sync-ccnx-wrapper.h"

using namespace Sync;
using namespace std;
using namespace boost;
using namespace ns3;


string echoStr = "";

void echo(string str) {
  echoStr = str;
}

struct TestStruct {
  string s_str1, s_str2;
  void set(string str1, string str2) {
    s_str1 = str1;
    s_str2 = str2;
  }
  int * num;
  int n;

  void rawSet(string str1, const char *buf, size_t len) {
    std::cout << "In rawSet" << std::endl;
    s_str1 = str1;

    n = len / 4; 
    num = new int [n];
    memcpy(num, buf, len);
  }
};


class CcnxWrapperTest
{
public:
  void
  step1 ()
  {
    globalFunc = echo;
    memberFunc = bind (&TestStruct::set, &foo, _1, _2);
    rawFunc = bind (&TestStruct::rawSet, &foo, _1, _2, _3);

    prefix = "/ucla.edu";
    ha.setInterestFilter(prefix, globalFunc);

    Simulator::Schedule (Seconds (0.01), &CcnxWrapperTest::step2, this);
  }

  void
  step2 ()
  {
    interest = "/ucla.edu/0";
    hb.sendInterestForString(interest, memberFunc);

    // give time for ndnSIM to react
    Simulator::Schedule (Seconds (1.005), &CcnxWrapperTest::step3, this);
  }

  void
  step3 ()
  {
    BOOST_CHECK_EQUAL(echoStr, interest);

    name = "/ucla.edu/0";
    data = "random bits: !#$!@#$!";
    ha.publishStringData(name, data, 5);

    hb.sendInterestForString(interest, memberFunc);

    // give time for ndnSIM to react
    Simulator::Schedule (Seconds (0.005), &CcnxWrapperTest::step4, this);
  }

  void
  step4 ()
  {
    BOOST_CHECK_EQUAL(foo.s_str1, name);
    BOOST_CHECK_EQUAL(foo.s_str2, data);

    rawDataName = "/ucla.edu/1";
    ha.publishRawData(rawDataName, (const char *)num, sizeof(num), 30);
    hb.sendInterest(rawDataName, rawFunc);

    // give time for ndnSIM to react
    Simulator::Schedule (Seconds (0.005), &CcnxWrapperTest::step5, this);
  }

  void
  step5 ()
  {
    BOOST_CHECK_EQUAL(foo.s_str1, rawDataName);
    for (int i = 0; i < 5; i++) {
      BOOST_CHECK(foo.num[i] == num[i]);
    }
  }
  
private:
  CcnxWrapper ha;
  CcnxWrapper hb;  

  TestStruct foo;

  boost::function<void (string)> globalFunc;
  boost::function<void (string, string)> memberFunc;
  boost::function<void (string, const char *, size_t)> rawFunc;

  string prefix;
  string interest;

  string name;
  string data;

  string rawDataName;
  const static int num [5];
};

int CcnxWrapperTest::num [] = {0, 1, 2, 3, 4};


boost::unit_test::test_suite *suite = BOOST_TEST_SUITE("TestCcnxWrapper");
// BOOST_AUTO_TEST_SUITE_END()

boost::shared_ptr<CcnxWrapperTest> instance( new CcnxWrapperTest );
suite->add( BOOST_CLASS_TEST_CASE (&CcnxWrapperTest::step1, instance) );
