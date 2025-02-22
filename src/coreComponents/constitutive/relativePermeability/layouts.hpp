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
 * @file layouts.hpp
 */

#ifndef GEOS_CONSTITUTIVE_RELATIVEPERMEABILITY_LAYOUTS_HPP
#define GEOS_CONSTITUTIVE_RELATIVEPERMEABILITY_LAYOUTS_HPP

#include "common/GeosxConfig.hpp"

#include "LvArray/src/typeManipulation.hpp"
#include "RAJA/RAJA.hpp"

namespace geos
{
namespace constitutive
{
namespace relperm
{

#if defined( GEOS_USE_DEVICE )

/// Constitutive model phase relative permeability array layout
using LAYOUT_RELPERM = RAJA::PERM_JKI;
/// Constitutive model phase relative permeability saturation derivative array layout
using LAYOUT_RELPERM_DS = RAJA::PERM_JKLI;

#else

/// Constitutive model phase relative permeability array layout
using LAYOUT_RELPERM = RAJA::PERM_IJK;
/// Constitutive model phase relative permeability saturation derivative array layout
using LAYOUT_RELPERM_DS = RAJA::PERM_IJKL;

#endif

/// Constitutive model phase relative permeability unit stride dimension
static constexpr int USD_RELPERM = LvArray::typeManipulation::getStrideOneDimension( LAYOUT_RELPERM{} );
/// Constitutive model phase relative permeability saturation derivative unit stride dimension
static constexpr int USD_RELPERM_DS = LvArray::typeManipulation::getStrideOneDimension( LAYOUT_RELPERM_DS{} );

} // namespace relperm
} // namespace constitutive
} // namespace geos

#endif //GEOS_CONSTITUTIVE_RELATIVEPERMEABILITY_LAYOUTS_HPP
