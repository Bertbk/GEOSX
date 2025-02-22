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
 * @file ToElementRelation.cpp
 */

#include "ToElementRelation.hpp"

#include "ElementRegionManager.hpp"

namespace geos
{


void erase( OrderedVariableToManyElementRelation & relation,
            localIndex const firstIndex,
            localIndex const er,
            localIndex const esr,
            localIndex const ei )
{
  for( localIndex a=relation.m_toElementRegion.sizeOfArray( firstIndex )-1; a>=0; --a )
  {
    if( er==relation.m_toElementRegion[firstIndex][a] &&
        esr==relation.m_toElementSubRegion[firstIndex][a] &&
        ei==relation.m_toElementIndex[firstIndex][a] )
    {
      relation.m_toElementRegion.eraseFromArray( firstIndex, a );
      relation.m_toElementSubRegion.eraseFromArray( firstIndex, a );
      relation.m_toElementIndex.eraseFromArray( firstIndex, a );
    }
  }
}

void insert( OrderedVariableToManyElementRelation & relation,
             localIndex const firstIndex,
             localIndex const er,
             localIndex const esr,
             localIndex const ei )
{
  bool alreadyPresent = false;
  for( localIndex a=0; a<relation.m_toElementRegion.sizeOfArray( firstIndex ); ++a )
  {
    if( er==relation.m_toElementRegion[firstIndex][a] &&
        esr==relation.m_toElementSubRegion[firstIndex][a] &&
        ei==relation.m_toElementIndex[firstIndex][a] )
    {
      alreadyPresent = true;
    }
  }
  if( !alreadyPresent )
  {
    relation.m_toElementRegion.emplaceBack( firstIndex, er );
    relation.m_toElementSubRegion.emplaceBack( firstIndex, esr );
    relation.m_toElementIndex.emplaceBack( firstIndex, ei );
  }
}

} /* namespace geos */
