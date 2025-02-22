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
 * @file HydraulicApertureBase.cpp
 */


#include "HydraulicApertureBase.hpp"


namespace geos
{

namespace constitutive
{

HydraulicApertureBase::HydraulicApertureBase( string const & name,
                                              Group * const parent ):
  ConstitutiveBase( name, parent ),
  m_aperture0( 0.0 )
{
  /// TODO: must become a required parameter.
  registerWrapper( viewKeyStruct::apertureZeroString(), &m_aperture0 ).
    setInputFlag( dataRepository::InputFlags::OPTIONAL ).
    setApplyDefaultValue( 1e-6 ).
    setDescription( "Reference hydraulic aperture. It is the aperture at zero normal stress." );
}

HydraulicApertureBase::~HydraulicApertureBase()
{}



} /* namespace constitutive */

} /* namespace geos */
