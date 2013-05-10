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

#ifdef NS3_MODULE

#include "sync-ns3-name-info.h"
#include "ns3/ndn-name.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <utility>

using namespace std;
using namespace boost;

namespace Sync {

NameInfoConstPtr
Ns3NameInfo::FindOrCreate (ns3::Ptr<const ns3::ndn::Name> name)
{
  mutex::scoped_lock namesLock (m_namesMutex);
  
  NameInfoConstPtr ret;
  string key = lexical_cast<string> (*name);
  
  NameMap::iterator item = m_names.find (key);
  if (item != m_names.end ())
    {
      ret = item->second.lock ();
      BOOST_ASSERT (ret != 0);
    }
  else
    {
      ret = NameInfoPtr (new Ns3NameInfo (name));
      weak_ptr<const NameInfo> value (ret);
      pair<NameMap::iterator,bool> inserted =
        m_names.insert (make_pair (key, value));
      
      BOOST_ASSERT (inserted.second); // previous call has to insert value
      item = inserted.first;
    }

  return ret;
}


Ns3NameInfo::Ns3NameInfo (ns3::Ptr<const ns3::ndn::Name> name)
  : m_name (name)
{
  m_id = m_ids ++; // set ID for a newly inserted element
  m_digest << *name;
  m_digest.getHash (); // finalize digest
}

string
Ns3NameInfo::toString () const
{
  return lexical_cast<std::string> (*m_name);
}

bool
Ns3NameInfo::operator == (const NameInfo &info) const
{
  try
    {
      return *m_name == *dynamic_cast<const Ns3NameInfo&> (info).m_name;
    }
  catch (...)
    {
      return false;
    }
}

bool
Ns3NameInfo::operator < (const NameInfo &info) const
{
  try
    {
      return *m_name < *dynamic_cast<const Ns3NameInfo&> (info).m_name;
    }
  catch (...)
    {
      return false;
    }
}

Digest &
operator << (Digest &digest, const ns3::ndn::Name &name)
{
  BOOST_FOREACH (const std::string &component, name.GetComponents ())
    {
      Digest subhash;
      subhash << component;
      subhash.getHash (); // finalize hash

      digest << subhash;
    }

  return digest;
}


} // Sync

#endif

