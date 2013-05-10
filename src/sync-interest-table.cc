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

#include "sync-interest-table.h"
#include "sync-log.h"
using namespace std;
using namespace boost;

INIT_LOGGER ("SyncInterestTable");

namespace Sync
{

SyncInterestTable::SyncInterestTable (TimeDuration lifetime)
  : m_entryLifetime (lifetime)
{
  m_scheduler.schedule (TIME_SECONDS (m_checkPeriod),
                        bind (&SyncInterestTable::expireInterests, this),
                        0);
}

SyncInterestTable::~SyncInterestTable ()
{
}

Interest
SyncInterestTable::pop ()
{
  recursive_mutex::scoped_lock lock (m_mutex);

  if (m_table.size () == 0)
    BOOST_THROW_EXCEPTION (Error::InterestTableIsEmpty ());

  Interest ret = *m_table.begin ();
  m_table.erase (m_table.begin ());

  return ret;
}

bool
SyncInterestTable::insert (DigestConstPtr digest, const string &name, bool unknownState/*=false*/)
{
  bool existent = false;
  
  recursive_mutex::scoped_lock lock (m_mutex);
  InterestContainer::index<named>::type::iterator it = m_table.get<named> ().find (name);
  if (it != m_table.end())
    {
      existent = true;
      m_table.erase (it);
    }
  m_table.insert (Interest (digest, name, unknownState));

  return existent;
}

uint32_t
SyncInterestTable::size () const
{
  recursive_mutex::scoped_lock lock (m_mutex);
  return m_table.size ();
}

bool
SyncInterestTable::remove (const string &name)
{
  recursive_mutex::scoped_lock lock (m_mutex);

  InterestContainer::index<named>::type::iterator item = m_table.get<named> ().find (name);
  if (item != m_table.get<named> ().end ())
    {
      m_table.get<named> ().erase (name);
      return true;
    }

  return false;
}

bool
SyncInterestTable::remove (DigestConstPtr digest)
{
  recursive_mutex::scoped_lock lock (m_mutex);
  InterestContainer::index<hashed>::type::iterator item = m_table.get<hashed> ().find (digest);
  if (item != m_table.get<hashed> ().end ())
    {
      m_table.get<hashed> ().erase (digest); // erase all records associated with the digest
      return true;
    }
  return false;
}

void SyncInterestTable::expireInterests ()
{ 
  recursive_mutex::scoped_lock lock (m_mutex);

  uint32_t count = 0;
  TimeAbsolute expireTime = TIME_NOW - m_entryLifetime;
  
  while (m_table.size () > 0)
    {
      InterestContainer::index<timed>::type::iterator item = m_table.get<timed> ().begin ();
      
      if (item->m_time < expireTime)
        {
          m_table.get<timed> ().erase (item);
          count ++;
        }
      else
        break;
  }

  _LOG_DEBUG ("expireInterests (): expired " << count);
  
  m_scheduler.schedule (TIME_SECONDS (m_checkPeriod),
                        bind (&SyncInterestTable::expireInterests, this),
                        0);
}


}
