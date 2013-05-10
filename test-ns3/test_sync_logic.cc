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
 *	   Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp> 
#include <map>
using boost::test_tools::output_test_stream;

#include <boost/make_shared.hpp>

#include "sync-ccnx-wrapper.h"
#include "sync-logic.h"
#include "sync-seq-no.h"

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ndnSIM-module.h>

using namespace std;
using namespace boost;
using namespace Sync;
using namespace ns3;

struct Handler
{
  string instance;
  
  Handler (const string &_instance)
  : instance (_instance)
  {
  }

  void wrapper (const vector<MissingDataInfo> &v) {
    int n = v.size();
    for (int i = 0; i < n; i++) {
      onUpdate (v[i].prefix, v[i].high, v[i].low);
    }
  }

  void onUpdate (const string &p/*prefix*/, const SeqNo &seq/*newSeq*/, const SeqNo &oldSeq/*oldSeq*/)
  {
    m_map[p] = seq.getSeq ();
    
    // cout << instance << "\t";
    // if (!oldSeq.isValid ())
    //   cout << "Inserted: " << p << " (" << seq << ")" << endl;
    // else
    //   cout << "Updated: " << p << "  ( " << oldSeq << ".." << seq << ")" << endl;
  }

  void onRemove (const string &p/*prefix*/)
  {
    // cout << instance << "\tRemoved: " << p << endl;
    m_map.erase (p);
  }

  map<string, uint32_t> m_map;
};

class SyncLogicFixture
{
public:
  SyncLogicFixture ()
    : h1 ("1")
    , h2 ("2")
  {
  }
  
  void
  step1 ()
  {
    Config::SetDefault ("ns3::ndn::ForwardingStrategy::CacheUnsolicitedData", BooleanValue (true));
    
    Ptr<Node> node = CreateObject<Node> ();
    ndn::StackHelper ndn;
    ndn.Install (node);

    l1 = CreateObject<SyncLogic> ("/bcast", bind (&Handler::wrapper, &h1, _1), bind (&Handler::onRemove, &h1, _1));
    l1->SetStartTime (Seconds (0));

    l2 = CreateObject<SyncLogic> ("/bcast", bind (&Handler::wrapper, &h2, _1), bind (&Handler::onRemove, &h2, _1));
    l2->SetStartTime (Seconds (1));
    
    node->AddApplication (l1);
    node->AddApplication (l2);

    Simulator::Schedule (Seconds (0.5), &SyncLogicFixture::step2, this);
    
    Simulator::Stop (Seconds (20.0));
    Simulator::Run ();
    Simulator::Destroy ();
  }
  
  void
  step2 ()
  {
    oldDigest  = l1->getRootDigest ();
    
    l1->addLocalNames ("/one", 1, 2);
    BOOST_CHECK_EQUAL (h1.m_map.size (), 0);
    Simulator::Schedule (Seconds (0.49), &SyncLogicFixture::step3, this);
  }

  void
  step3 ()
  {
    BOOST_CHECK_EQUAL (h1.m_map.size (), 0);

    Simulator::Schedule (Seconds (1), &SyncLogicFixture::step4, this);
  }

  void
  step4 ()
  {
    BOOST_CHECK_EQUAL (h1.m_map.size (), 0);
    BOOST_CHECK_EQUAL (h2.m_map.size (), 1);
    
    l1->remove ("/one");
    Simulator::Schedule (Seconds (1), &SyncLogicFixture::step5, this);
  }

  void
  step5 ()
  {
    std::string newDigest = l1->getRootDigest ();
    BOOST_CHECK_EQUAL (oldDigest, newDigest);
  }
  
private:
  Handler h1;
  Ptr<SyncLogic> l1;

  Handler h2;
  Ptr<SyncLogic> l2;

  std::string oldDigest;
};


BOOST_FIXTURE_TEST_SUITE( SyncLogicTest, SyncLogicFixture )

BOOST_AUTO_TEST_CASE( test_case1 )
{
  step1 ();
}

BOOST_AUTO_TEST_SUITE_END()
