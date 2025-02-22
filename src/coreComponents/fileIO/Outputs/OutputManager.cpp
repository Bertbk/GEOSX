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
 * @file OutputManager.cpp
 */

#include "OutputManager.hpp"
#include "SiloOutput.hpp"

namespace geos
{

using namespace dataRepository;

OutputManager::OutputManager( string const & name,
                              Group * const parent ):
  Group( name, parent )
{
  setInputFlags( InputFlags::REQUIRED );
}

OutputManager::~OutputManager()
{}



Group * OutputManager::createChild( string const & childKey, string const & childName )
{
  GEOS_LOG_RANK_0( GEOS_FMT( "{}: adding {} {}", getName(), childKey, childName ) );
  std::unique_ptr< OutputBase > output = OutputBase::CatalogInterface::factory( childKey, childName, this );
  return &this->registerGroup< OutputBase >( childName, std::move( output ) );
}


void OutputManager::expandObjectCatalogs()
{
  // During schema generation, register one of each type derived from OutputBase here
  for( auto & catalogIter: OutputBase::getCatalog() )
  {
    createChild( catalogIter.first, catalogIter.first );
  }
}

} /* namespace geos */
