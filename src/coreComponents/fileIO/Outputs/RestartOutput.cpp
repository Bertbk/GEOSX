/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2016-2024 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2024 TotalEnergies
 * Copyright (c) 2018-2024 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2023-2024 Chevron
 * Copyright (c) 2019-     GEOS/GEOSX Contributors
 * All rights reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file RestartOutput.cpp
 */

#include "RestartOutput.hpp"

namespace geos
{

using namespace dataRepository;

namespace logInfo
{
struct RestartOutputTimer : public OutputTimerBase
{
  std::string_view getDescription() const override { return "Restart output timing"; }
};
}

RestartOutput::RestartOutput( string const & name,
                              Group * const parent ):
  OutputBase( name, parent )
{}

RestartOutput::~RestartOutput()
{}

bool RestartOutput::execute( real64 const GEOS_UNUSED_PARAM( time_n ),
                             real64 const GEOS_UNUSED_PARAM( dt ),
                             integer const cycleNumber,
                             integer const GEOS_UNUSED_PARAM( eventCounter ),
                             real64 const GEOS_UNUSED_PARAM( eventProgress ),
                             DomainPartition & GEOS_UNUSED_PARAM( domain ) )
{
  GEOS_MARK_FUNCTION;

  {
    Timer timer( m_outputTimer );

    Group & rootGroup = this->getGroupByPath( "/Problem" );
    string const fileName = GEOS_FMT( "{}_restart_{:09}", getFileNameRoot(), cycleNumber );
    rootGroup.prepareToWrite();
    writeTree( joinPath( OutputBase::getOutputDirectory(), fileName ), *(rootGroup.getConduitNode().parent()) );
    rootGroup.finishWriting();
  }

  return false;
}

logInfo::OutputTimerBase const & RestartOutput::getTimerCategory() const
{
  static logInfo::RestartOutputTimer timer;
  return timer;
}

REGISTER_CATALOG_ENTRY( OutputBase, RestartOutput, string const &, Group * const )
} /* namespace geos */
