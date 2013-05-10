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

#include "sync-log.h"

using namespace Sync;
using namespace std;
using namespace boost;
using namespace ns3;

INIT_LOGGER ("Test.CcnxWrapper");

string echoStr = "";

void echo(string str) {
  _LOG_DEBUG ("<< I " << str);
  echoStr = str;
}

struct TestStruct {
  TestStruct ()
    : num (0)
  {
  }
  
  string s_str1, s_str2;
  void set(string str1, string str2) {
    s_str1 = str1;
    s_str2 = str2;
  }
  int * num;
  int n;

  void rawSet(string str1, const char *buf, size_t len)
  {
    // std::cout << "In rawSet" << std::endl;
    _LOG_DEBUG ("In rawSet");
    s_str1 = str1;

    n = len / 4; 
    num = new int [n];
    memcpy(num, buf, len);
  }
};


class WrapperFixture
{
public:
  void
  step1 ()
  {
    Config::SetDefault ("ns3::ndn::ForwardingStrategy::CacheUnsolicitedData", BooleanValue (true));
    
    Ptr<Node> node = CreateObject<Node> ();
    ndn::StackHelper ndn;
    ndn.Install (node);

    ha = CreateObject<CcnxWrapper> ();
    hb = CreateObject<CcnxWrapper> ();

    node->AddApplication (ha);
    node->AddApplication (hb);

    Simulator::Schedule (Seconds (1.0), &WrapperFixture::step1_5, this);
    
    Simulator::Stop (Seconds (20.0));
    Simulator::Run ();
    Simulator::Destroy ();
  }

  void
  step1_5 ()
  {
    globalFunc = echo;

    prefix = "/ucla.edu";
    ha->setInterestFilter (prefix, globalFunc);

    Simulator::Schedule (Seconds (0.01), &WrapperFixture::step2, this);
  }
  
  void
  step2 ()
  {
    memberFunc = bind (&TestStruct::set, &foo, _1, _2);
    interest = "/ucla.edu/0";
    hb->sendInterestForString (interest, memberFunc);

    // give time for ndnSIM to react
    Simulator::Schedule (Seconds (1.005), &WrapperFixture::step3, this);
  }

  void
  step3 ()
  {
    BOOST_CHECK_EQUAL(echoStr, interest);

    name = "/ucla.edu/0";
    data = "random bits: !#$!@#$!";
    ha->publishStringData(name, data, 5);

    hb->sendInterestForString(interest, memberFunc);

    // give time for ndnSIM to react
    Simulator::Schedule (Seconds (0.005), &WrapperFixture::step4, this);
  }

  void
  step4 ()
  {
    BOOST_CHECK_EQUAL(foo.s_str1, name);
    BOOST_CHECK_EQUAL(foo.s_str2, data);

    rawDataName = "/ucla.edu/1";
    ha->publishRawData(rawDataName, (const char *)num, sizeof(num), 30);
    rawFunc = bind (&TestStruct::rawSet, &foo, _1, _2, _3);
    hb->sendInterest(rawDataName, rawFunc);

    // // give time for ndnSIM to react
    Simulator::Schedule (Seconds (0.005), &WrapperFixture::step5, this);
  }

  void
  step5 ()
  {
    BOOST_CHECK_EQUAL(foo.s_str1, rawDataName);
    BOOST_CHECK_NE (foo.num, (int *)0);
    if (foo.num)
      {
        for (int i = 0; i < 5; i++)
          {
            BOOST_CHECK(foo.num[i] == num[i]);
          }
      }
  }
  
private:
  Ptr<CcnxWrapper> ha;
  Ptr<CcnxWrapper> hb;  

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

int WrapperFixture::num [] = {0, 1, 2, 3, 4};

BOOST_FIXTURE_TEST_SUITE( TestCcnxWrapper, WrapperFixture )

BOOST_AUTO_TEST_CASE( test_case1 )
{
  step1 ();
}

BOOST_AUTO_TEST_SUITE_END()
