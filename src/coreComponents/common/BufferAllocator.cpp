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

#include "BufferAllocator.hpp"
#include "DataTypes.hpp"

#ifdef GEOS_USE_CHAI
namespace geos
{

bool prefer_pinned_buffer = true;

void setPreferPinned( bool p )
{
  prefer_pinned_buffer = p;
}

bool getPreferPinned( )
{
  return prefer_pinned_buffer;
}

}

#endif
