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
 *	       Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "sync-ccnx-wrapper.h"
#include "sync-log.h"

#include <boost/throw_exception.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

#include <ns3/packet.h>

#include <ns3/ndn-face.h>
#include <ns3/ndn-fib.h>

typedef boost::error_info<struct tag_errmsg, std::string> errmsg_info_str;
typedef boost::error_info<struct tag_errmsg, int> errmsg_info_int;

using namespace std;
using namespace boost;
using namespace ns3;

INIT_LOGGER ("CcnxWrapper");

namespace Sync {

CcnxWrapper::CcnxWrapper()
  : m_rand (0, std::numeric_limits<uint32_t>::max ())
{
}

CcnxWrapper::~CcnxWrapper()
{
}

void
CcnxWrapper::StartApplication ()
{
  ndn::App::StartApplication ();
}

void
CcnxWrapper::StopApplication ()
{
  ndn::App::StopApplication ();
}

int
CcnxWrapper::publishRawData (const std::string &name, const char *buf, size_t len, int freshness)
{
  _LOG_INFO (">> publishRawData " << name);
  
  Ptr<ndn::Data> data = Create<ndn::Data> (Create<Packet> (reinterpret_cast<const uint8_t*> (buf), len));
  Ptr<ndn::Name> dataName = Create<ndn::Name> (name);
  data->SetName (dataName);
  data->SetFreshness (Seconds (freshness));

  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);

  return 0;
}


void
RawDataCallback2StringDataCallback (CcnxWrapper::StringDataCallback callback, std::string str, const char *buf, size_t len)
{
  callback (str, string (buf, len));
}

int
CcnxWrapper::sendInterestForString (const std::string &strInterest, const StringDataCallback &strDataCallback/*, int retry*/)
{
  return sendInterest (strInterest, boost::bind (RawDataCallback2StringDataCallback, strDataCallback, _1, _2, _3));
}

int CcnxWrapper::sendInterest (const string &strInterest, const RawDataCallback &rawDataCallback)
{
  _LOG_INFO (">> Requesting Interest: " << strInterest);
  Ptr<ndn::Name> name = Create<ndn::Name> (strInterest);

  Ptr<ndn::Interest> interest = Create<ndn::Interest> ();
  interest->SetNonce            (m_rand.GetValue ());
  interest->SetName             (*name);
  interest->SetInterestLifetime (Seconds (9.9)); // really long-lived interests
  
  // Record the callback
  CcnxFilterEntryContainer<RawDataCallback>::iterator entry = m_dataCallbacks.find_exact (*name);
  if (entry == m_dataCallbacks.end ())
    {
      pair<CcnxFilterEntryContainer<RawDataCallback>::iterator, bool> status =
        m_dataCallbacks.insert (*name, Create< CcnxFilterEntry<RawDataCallback> > (name));

      entry = status.first;
    }
  entry->payload ()->AddCallback (rawDataCallback);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
  
  return 0;
}

int CcnxWrapper::setInterestFilter (const string &prefix, const InterestCallback &interestCallback)
{
  //NS_LOG_INFO ("== setInterestFilter " << prefix << " (" << GetNode ()->GetId () << ")");
  Ptr<ndn::Name> name = Create<ndn::Name> (prefix);

  CcnxFilterEntryContainer<InterestCallback>::iterator entry = m_interestCallbacks.find_exact (*name);
  if (entry == m_interestCallbacks.end ())
    {
      pair<CcnxFilterEntryContainer<InterestCallback>::iterator, bool> status =
        m_interestCallbacks.insert (*name, Create < CcnxFilterEntry<InterestCallback> > (name));

      entry = status.first;
    }

  entry->payload ()->AddCallback (interestCallback);

  // creating actual face
  Ptr<ndn::Fib> fib = GetNode ()->GetObject<ndn::Fib> ();
  Ptr<ndn::fib::Entry> fibEntry = fib->Add (name, m_face, 0);
  fibEntry->UpdateStatus (m_face, ndn::fib::FaceMetric::NDN_FIB_GREEN);

  return 0;
}

void
CcnxWrapper::clearInterestFilter (const std::string &prefix)
{
  Ptr<ndn::Name> name = Create<ndn::Name> (prefix);

  CcnxFilterEntryContainer<InterestCallback>::iterator entry = m_interestCallbacks.find_exact (*name);
  if (entry == m_interestCallbacks.end ())
    return;

  entry->payload ()->ClearCallback ();
}

void
CcnxWrapper::OnInterest (ns3::Ptr<const ndn::Interest> interest)
{
  ndn::App::OnInterest (interest);

  // the app cannot set several filters for the same prefix
  CcnxFilterEntryContainer<InterestCallback>::iterator entry = m_interestCallbacks.longest_prefix_match (interest->GetName ());
  if (entry == m_interestCallbacks.end ())
    {
      _LOG_DEBUG ("No Interest callback set");
      return;
    }
  
  entry->payload ()->m_callback (lexical_cast<string> (interest->GetName ()));  
}

void
CcnxWrapper::OnData (ns3::Ptr<const ndn::Data> contentObject)
{
  ndn::App::OnData (contentObject);
  //NS_LOG_DEBUG ("<< D " << contentObject->GetName ());

  CcnxFilterEntryContainer<RawDataCallback>::iterator entry = m_dataCallbacks.longest_prefix_match (contentObject->GetName ());
  if (entry == m_dataCallbacks.end ())
    {
      _LOG_DEBUG ("No Data callback set");
      return;
    }

  while (entry != m_dataCallbacks.end ())
    {
      ostringstream content;
      contentObject->GetPayload()->CopyData (&content, contentObject->GetPayload()->GetSize ());
  
      entry->payload ()->m_callback (lexical_cast<string> (contentObject->GetName ()), content.str ().c_str (), content.str ().size ());
  
      m_dataCallbacks.erase (entry);

      entry = m_dataCallbacks.longest_prefix_match (contentObject->GetName ());
    }
}

}
