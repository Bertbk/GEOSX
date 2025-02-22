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

#include "PythonOutput.hpp"

namespace geos
{

namespace logInfo
{
struct PythonOutputTimer : public OutputTimerBase
{
  std::string_view getDescription() const override { return "Python output timing"; }
};
}

logInfo::OutputTimerBase const & PythonOutput::getTimerCategory() const
{
  static logInfo::PythonOutputTimer timer;
  return timer;
}

REGISTER_CATALOG_ENTRY( OutputBase, PythonOutput, string const &, dataRepository::Group * const )

} // namespace geos
