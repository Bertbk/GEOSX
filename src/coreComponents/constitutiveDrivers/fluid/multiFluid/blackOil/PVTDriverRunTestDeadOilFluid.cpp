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

/*
 * PVTDriverRunTestDeadOilFluid.cpp
 */

#include "constitutiveDrivers/fluid/multiFluid/PVTDriverRunTest.hpp"
#include "constitutive/fluid/multifluid/blackOil/DeadOilFluid.hpp"
#include "constitutive/fluid/multifluid/blackOil/BlackOilFluid.hpp"


namespace geos
{
template void PVTDriver::runTest< constitutive::DeadOilFluid >( constitutive::DeadOilFluid &, arrayView2d< real64 > const & );
template void PVTDriver::runTest< constitutive::BlackOilFluid >( constitutive::BlackOilFluid &, arrayView2d< real64 > const & );
}
