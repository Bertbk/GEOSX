# SPDX-License-Identifier: LGPL-2.1-only
#
# Copyright (c) 2016-2024 Lawrence Livermore National Security LLC
# Copyright (c) 2018-2024 TotalEnergies
# Copyright (c) 2018-2024 The Board of Trustees of the Leland Stanford Junior University
# Copyright (c) 2023-2024 Chevron
# Copyright (c) 2019-     GEOS/GEOSX Contributors
# All rights reserved
#
# See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
#
#--------------------------------------------------------------------------------------------------

#[[
Package: finiteVolume

Implementation and access to finite volume method. The main data structures of this package are
 the stencils: connection values at the interfaces of the volume elements.
#]]

#
# Specify all headers
#
set( finiteVolume_headers
     BoundaryStencil.hpp
     CellElementStencilMPFA.hpp
     CellElementStencilTPFA.hpp
     SurfaceElementStencil.hpp
     EmbeddedSurfaceToCellStencil.hpp
     FaceElementToCellStencil.hpp
     FiniteVolumeManager.hpp
     FluxApproximationBase.hpp
     ProjectionEDFMHelper.hpp
     HybridMimeticDiscretization.hpp
     MimeticInnerProductDispatch.hpp
     StencilBase.hpp
     TwoPointFluxApproximation.hpp
     mimeticInnerProducts/BdVLMInnerProduct.hpp
     mimeticInnerProducts/MimeticInnerProductBase.hpp
     mimeticInnerProducts/MimeticInnerProductHelpers.hpp
     mimeticInnerProducts/QuasiRTInnerProduct.hpp
     mimeticInnerProducts/QuasiTPFAInnerProduct.hpp
     mimeticInnerProducts/SimpleInnerProduct.hpp
     mimeticInnerProducts/TPFAInnerProduct.hpp
   )
#
# Specify all sources
#
set( finiteVolume_sources
     BoundaryStencil.cpp
     CellElementStencilMPFA.cpp
     CellElementStencilTPFA.cpp
     SurfaceElementStencil.cpp
     FaceElementToCellStencil.cpp
     EmbeddedSurfaceToCellStencil.cpp
     FiniteVolumeManager.cpp
     FluxApproximationBase.cpp
     TwoPointFluxApproximation.cpp
     ProjectionEDFMHelper.cpp
     HybridMimeticDiscretization.cpp
   )

set( dependencyList ${parallelDeps} mesh fieldSpecification )

geos_decorate_link_dependencies( LIST decoratedDependencies
                                 DEPENDENCIES ${dependencyList} )

blt_add_library( NAME       finiteVolume
                 SOURCES    ${finiteVolume_sources}
                 HEADERS    ${finiteVolume_headers}
                 DEPENDS_ON ${decoratedDependencies}
                 OBJECT     ${GEOS_BUILD_OBJ_LIBS}
                 SHARED     ${GEOS_BUILD_SHARED_LIBS}
               )

target_include_directories( finiteVolume PUBLIC ${CMAKE_SOURCE_DIR}/coreComponents )

install( TARGETS finiteVolume LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
