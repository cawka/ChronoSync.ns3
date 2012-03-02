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
 *         卞超轶 Chaoyi Bian <bcy@pku.edu.cn>
 *	   Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "sync-ccnx-name-info.h"
#include "ns3/ccnx-name-components.h"

#include <boost/lexical_cast.hpp>
#include <utility>

using namespace std;
using namespace boost;

namespace ns3 {
namespace Sync {

NameInfoConstPtr
CcnxNameInfo::FindOrCreate (Ptr<const CcnxNameComponents> name)
{
  string key = lexical_cast<string> (*name);

  NameInfoPtr value = NameInfoPtr (new CcnxNameInfo (name));
  pair<NameMap::iterator,bool> item =
    m_names.insert (make_pair (key, value));

  return item.first->second;
}

CcnxNameInfo::CcnxNameInfo (Ptr<const CcnxNameComponents> name)
  : m_name (name)
{
  m_id = m_ids ++; // set ID for a newly inserted element
}

string
CcnxNameInfo::toString () const
{
  return lexical_cast<std::string> (*m_name);
}

bool
CcnxNameInfo::operator == (const NameInfo &info) const
{
  try
    {
      return *m_name == *dynamic_cast<const CcnxNameInfo&> (info).m_name;
    }
  catch (...)
    {
      return false;
    }
}



} // Sync
} // ns3
