/*
 * ------------------------------------------------------------------------------------------------------------
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * Copyright (c) 2016-2024 Lawrence Livermore National Security LLC
 * Copyright (c) 2018-2024 Total, S.A
 * Copyright (c) 2018-2024 The Board of Trustees of the Leland Stanford Junior University
 * Copyright (c) 2023-2024 Chevron
 * Copyright (c) 2019-     GEOS/GEOSX Contributors
 * All rights reserved
 *
 * See top level LICENSE, COPYRIGHT, CONTRIBUTORS, NOTICE, and ACKNOWLEDGEMENTS files for details.
 * ------------------------------------------------------------------------------------------------------------
 */

/**
 * @file QuasiDynamicEQRK32.cpp
 */

#include "QuasiDynamicEQRK32.hpp"

#include "dataRepository/InputFlags.hpp"
#include "mesh/DomainPartition.hpp"
#include "kernels/RateAndStateKernels.hpp"
#include "rateAndStateFields.hpp"
#include "physicsSolvers/contact/ContactFields.hpp"
#include "fieldSpecification/FieldSpecificationManager.hpp"

namespace geos
{

using namespace dataRepository;
using namespace fields;
using namespace constitutive;
using namespace rateAndStateKernels;

QuasiDynamicEQRK32::QuasiDynamicEQRK32( const string & name,
                                        Group * const parent ):
  PhysicsSolverBase( name, parent ),
  m_stressSolver( nullptr ),
  m_stressSolverName( "SpringSlider" ),
  m_shearImpedance( 0.0 ),
  m_butcherTable( BogackiShampine32Table()), // TODO: The butcher table should be specified in the XML input.
  m_successfulStep( false ),
  m_controller( PIDController( { 1.0/18.0, 1.0/9.0, 1.0/18.0 },
                               1.0e-6, 1.0e-6, 0.81 )) // TODO: The control parameters should be specified in the XML input
{
  this->registerWrapper( viewKeyStruct::shearImpedanceString(), &m_shearImpedance ).
    setInputFlag( InputFlags::REQUIRED ).
    setDescription( "Shear impedance." );

  this->registerWrapper( viewKeyStruct::stressSolverNameString(), &m_stressSolverName ).
    setInputFlag( InputFlags::OPTIONAL ).
    setDescription( "Name of solver for computing stress. If empty, the spring-slider model is run." );
}

void QuasiDynamicEQRK32::postInputInitialization()
{

  // Initialize member stress solver as specified in XML input
  if( !m_stressSolverName.empty() )
  {
    m_stressSolver = &this->getParent().getGroup< PhysicsSolverBase >( m_stressSolverName );
  }

  PhysicsSolverBase::postInputInitialization();
}

QuasiDynamicEQRK32::~QuasiDynamicEQRK32()
{
  // TODO Auto-generated destructor stub
}


void QuasiDynamicEQRK32::registerDataOnMesh( Group & meshBodies )
{
  PhysicsSolverBase::registerDataOnMesh( meshBodies );

  forDiscretizationOnMeshTargets( meshBodies, [&] ( string const &,
                                                    MeshLevel & mesh,
                                                    arrayView1d< string const > const & regionNames )
  {
    ElementRegionManager & elemManager = mesh.getElemManager();

    elemManager.forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                 [&]( localIndex const,
                                                                      SurfaceElementSubRegion & subRegion )
    {
      // Scalar functions on fault
      subRegion.registerField< rateAndState::stateVariable >( getName() );
      subRegion.registerField< rateAndState::stateVariable_n >( getName() );
      subRegion.registerField< rateAndState::slipRate >( getName() );

      // Tangent (2-component) functions on fault
      string const labels2Comp[2] = {"tangent1", "tangent2" };
      subRegion.registerField< rateAndState::slipVelocity >( getName() ).
        setDimLabels( 1, labels2Comp ).reference().resizeDimension< 1 >( 2 );
      subRegion.registerField< rateAndState::slipVelocity_n >( getName() ).
        setDimLabels( 1, labels2Comp ).reference().resizeDimension< 1 >( 2 );
      subRegion.registerField< rateAndState::deltaSlip >( getName() ).
        setDimLabels( 1, labels2Comp ).reference().resizeDimension< 1 >( 2 );
      subRegion.registerField< rateAndState::deltaSlip_n >( getName() ).
        setDimLabels( 1, labels2Comp ).reference().resizeDimension< 1 >( 2 );

      // Runge-Kutta stage rates and error
      integer const numRKComponents = 3;
      subRegion.registerField< rateAndState::rungeKuttaStageRates >( getName() ).reference().resizeDimension< 1, 2 >( m_butcherTable.numStages, numRKComponents );
      subRegion.registerField< rateAndState::error >( getName() ).reference().resizeDimension< 1 >( numRKComponents );


      if( !subRegion.hasWrapper( contact::dispJump::key() ))
      {
        // 3-component functions on fault
        string const labels3Comp[3] = { "normal", "tangent1", "tangent2" };
        subRegion.registerField< contact::dispJump >( getName() ).
          setDimLabels( 1, labels3Comp ).
          reference().resizeDimension< 1 >( 3 );
        subRegion.registerField< contact::dispJump_n >( getName() ).
          setDimLabels( 1, labels3Comp ).
          reference().resizeDimension< 1 >( 3 );
        subRegion.registerField< contact::traction >( getName() ).
          setDimLabels( 1, labels3Comp ).
          reference().resizeDimension< 1 >( 3 );
        subRegion.registerField< contact::traction_n >( getName() ).
          setDimLabels( 1, labels3Comp ).
          reference().resizeDimension< 1 >( 3 );

        subRegion.registerWrapper< string >( viewKeyStruct::frictionLawNameString() ).
          setPlotLevel( PlotLevel::NOPLOT ).
          setRestartFlags( RestartFlags::NO_WRITE ).
          setSizedFromParent( 0 );

        string & frictionLawName = subRegion.getReference< string >( viewKeyStruct::frictionLawNameString() );
        frictionLawName = PhysicsSolverBase::getConstitutiveName< FrictionBase >( subRegion );
        GEOS_ERROR_IF( frictionLawName.empty(), GEOS_FMT( "{}: FrictionBase model not found on subregion {}",
                                                          getDataContext(), subRegion.getDataContext() ) );
      }
    } );
  } );
}

real64 QuasiDynamicEQRK32::solverStep( real64 const & time_n,
                                       real64 const & dt,
                                       int const cycleNumber,
                                       DomainPartition & domain )
{
  if( cycleNumber == 0 )
  {
    /// Apply initial conditions to the Fault
    FieldSpecificationManager & fieldSpecificationManager = FieldSpecificationManager::getInstance();

    forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                                 MeshLevel & mesh,
                                                                 arrayView1d< string const > const & )

    {
      fieldSpecificationManager.applyInitialConditions( mesh );

    } );
    saveState( domain );
  }

  real64 dtAdaptive = dt;

  GEOS_LOG_LEVEL_RANK_0( 1, "Begin adaptive time step" );
  while( true ) // Adaptive time step loop. Performs a Runge-Kutta time stepping with error control on state and slip
  {
    real64 dtStress; GEOS_UNUSED_VAR( dtStress );

    // Initial Runge-Kutta stage
    stepRateStateODEInitialSubstage( dtAdaptive, domain );
    real64 dtStage = m_butcherTable.c[1]*dtAdaptive;
    dtStress = updateStresses( time_n, dtStage, cycleNumber, domain );
    updateSlipVelocity( time_n, dtStage, domain );

    // Remaining stages
    for( integer stageIndex = 1; stageIndex < m_butcherTable.numStages-1; stageIndex++ )
    {
      stepRateStateODESubstage( stageIndex, dtAdaptive, domain );
      dtStage = m_butcherTable.c[stageIndex+1]*dtAdaptive;
      dtStress = updateStresses( time_n, dtStage, cycleNumber, domain );
      updateSlipVelocity( time_n, dtStage, domain );
    }

    stepRateStateODEAndComputeError( dtAdaptive, domain );
    // Update timestep based on the time step error
    real64 const dtNext = setNextDt( dtAdaptive, domain );
    if( m_successfulStep ) // set in setNextDt
    {
      // Compute stresses, and slip velocity and save results at updated time,
      if( !m_butcherTable.FSAL )
      {
        dtStress = updateStresses( time_n, dtAdaptive, cycleNumber, domain );
        updateSlipVelocity( time_n, dtAdaptive, domain );
      }
      saveState( domain );
      // update the time step and exit the adaptive time step loop
      dtAdaptive = dtNext;
      break;
    }
    else
    {
      // Retry with updated time step
      dtAdaptive = dtNext;
    }
  }
  // return time step size achieved by stress solver
  return dtAdaptive;
}

void QuasiDynamicEQRK32::stepRateStateODEInitialSubstage( real64 const dt, DomainPartition & domain ) const
{

  forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                               MeshLevel & mesh,
                                                               arrayView1d< string const > const & regionNames )

  {
    mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                           [&]( localIndex const,
                                                                                SurfaceElementSubRegion & subRegion )
    {

      string const & fricitonLawName = subRegion.template getReference< string >( viewKeyStruct::frictionLawNameString() );
      RateAndStateFriction const & frictionLaw = getConstitutiveModel< RateAndStateFriction >( subRegion, fricitonLawName );
      rateAndStateKernels::EmbeddedRungeKuttaKernel rkKernel( subRegion, frictionLaw, m_butcherTable );
      arrayView3d< real64 > const rkStageRates      = subRegion.getField< rateAndState::rungeKuttaStageRates >();

      if( m_butcherTable.FSAL && m_successfulStep )
      {
        forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
        {
          rkKernel.updateStageRatesFSAL( k );
          rkKernel.updateStageValues( k, 1, dt );
        } );
      }
      else
      {
        forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
        {
          rkKernel.initialize( k );
          rkKernel.updateStageRates( k, 0 );
          rkKernel.updateStageValues( k, 1, dt );
        } );
      }
    } );
  } );
}

void QuasiDynamicEQRK32::stepRateStateODESubstage( integer const stageIndex,
                                                   real64 const dt,
                                                   DomainPartition & domain ) const
{

  forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                               MeshLevel & mesh,
                                                               arrayView1d< string const > const & regionNames )

  {
    mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                           [&]( localIndex const,
                                                                                SurfaceElementSubRegion & subRegion )
    {

      string const & fricitonLawName = subRegion.template getReference< string >( viewKeyStruct::frictionLawNameString() );
      RateAndStateFriction const & frictionLaw = getConstitutiveModel< RateAndStateFriction >( subRegion, fricitonLawName );
      rateAndStateKernels::EmbeddedRungeKuttaKernel rkKernel( subRegion, frictionLaw, m_butcherTable );
      arrayView3d< real64 > const rkStageRates      = subRegion.getField< rateAndState::rungeKuttaStageRates >();

      forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
      {
        rkKernel.updateStageRates( k, stageIndex );
        rkKernel.updateStageValues( k, stageIndex+1, dt );
      } );
    } );
  } );
}

void QuasiDynamicEQRK32::stepRateStateODEAndComputeError( real64 const dt, DomainPartition & domain ) const
{
  forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                               MeshLevel & mesh,
                                                               arrayView1d< string const > const & regionNames )

  {
    mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                           [&]( localIndex const,
                                                                                SurfaceElementSubRegion & subRegion )
    {

      string const & fricitonLawName = subRegion.template getReference< string >( viewKeyStruct::frictionLawNameString() );
      RateAndStateFriction const & frictionLaw = getConstitutiveModel< RateAndStateFriction >( subRegion, fricitonLawName );
      rateAndStateKernels::EmbeddedRungeKuttaKernel rkKernel( subRegion, frictionLaw, m_butcherTable );
      arrayView3d< real64 > const rkStageRates      = subRegion.getField< rateAndState::rungeKuttaStageRates >();
      if( m_butcherTable.FSAL )
      {
        forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
        {
          // Perform last stage rate update
          rkKernel.updateStageRates( k, m_butcherTable.numStages-1 );
          // Update solution to final time and compute errors
          rkKernel.updateSolutionAndLocalErrorFSAL( k, dt, m_controller.absTol, m_controller.relTol );
        } );
      }
      else
      {
        forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
        {
          // Perform last stage rate update
          rkKernel.updateStageRates( k, m_butcherTable.numStages-1 );
          // Update solution to final time and compute errors
          rkKernel.updateSolutionAndLocalError( k, dt, m_controller.absTol, m_controller.relTol );
        } );
      }
    } );
  } );
}

real64 QuasiDynamicEQRK32::updateStresses( real64 const & time_n,
                                           real64 const & dt,
                                           const int cycleNumber,
                                           DomainPartition & domain ) const
{
  GEOS_LOG_LEVEL_RANK_0( 1, "Stress solver" );
  // Call member variable stress solver to update the stress state
  if( m_stressSolver )
  {
    // 1. Solve the momentum balance
    real64 const dtStress =  m_stressSolver->solverStep( time_n, dt, cycleNumber, domain );

    return dtStress;
  }
  else
  {
    // Spring-slider shear traction computation
    forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                                 MeshLevel & mesh,
                                                                 arrayView1d< string const > const & regionNames )

    {
      mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                             [&]( localIndex const,
                                                                                  SurfaceElementSubRegion & subRegion )
      {

        arrayView2d< real64 const > const deltaSlip = subRegion.getField< rateAndState::deltaSlip >();
        arrayView2d< real64 > const traction        = subRegion.getField< fields::contact::traction >();
        arrayView2d< real64 const > const traction_n      = subRegion.getField< fields::contact::traction_n >();

        string const & fricitonLawName = subRegion.template getReference< string >( viewKeyStruct::frictionLawNameString() );
        RateAndStateFriction const & frictionLaw = getConstitutiveModel< RateAndStateFriction >( subRegion, fricitonLawName );

        RateAndStateFriction::KernelWrapper frictionKernelWrapper = frictionLaw.createKernelUpdates();

        forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
        {
          SpringSliderParameters springSliderParameters = SpringSliderParameters( traction[k][0],
                                                                                  frictionKernelWrapper.getACoefficient( k ),
                                                                                  frictionKernelWrapper.getBCoefficient( k ),
                                                                                  frictionKernelWrapper.getDcCoefficient( k ) );


          traction[k][1] = traction_n[k][1] + springSliderParameters.tauRate * dt
                           - springSliderParameters.springStiffness * deltaSlip[k][0];
          traction[k][2] = traction_n[k][2] + springSliderParameters.tauRate * dt
                           - springSliderParameters.springStiffness * deltaSlip[k][1];
        } );
      } );
    } );
    return dt;
  }
}

void QuasiDynamicEQRK32::updateSlipVelocity( real64 const & time_n,
                                             real64 const & dt,
                                             DomainPartition & domain ) const
{
  GEOS_LOG_LEVEL_RANK_0( 1, "Rate and State solver" );
  integer const maxIterNewton = m_nonlinearSolverParameters.m_maxIterNewton;
  real64 const newtonTol = m_nonlinearSolverParameters.m_newtonTol;
  forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                               MeshLevel & mesh,
                                                               arrayView1d< string const > const & regionNames )

  {
    mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                           [&]( localIndex const,
                                                                                SurfaceElementSubRegion & subRegion )
    {
      // solve rate and state equations.
      rateAndStateKernels::createAndLaunch< rateAndStateKernels::ExplicitRateAndStateKernel, parallelDevicePolicy<> >( subRegion, viewKeyStruct::frictionLawNameString(), m_shearImpedance,
                                                                                                                       maxIterNewton, newtonTol, time_n, dt );
    } );
  } );
}

void QuasiDynamicEQRK32::saveState( DomainPartition & domain ) const
{
  forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                               MeshLevel & mesh,
                                                               arrayView1d< string const > const & regionNames )

  {
    mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                           [&]( localIndex const,
                                                                                SurfaceElementSubRegion & subRegion )
    {
      arrayView1d< real64 const > const stateVariable = subRegion.getField< rateAndState::stateVariable >();
      arrayView2d< real64 const > const slipVelocity  = subRegion.getField< rateAndState::slipVelocity >();
      arrayView2d< real64 const > const deltaSlip     = subRegion.getField< rateAndState::deltaSlip >();
      arrayView2d< real64 const > const dispJump      = subRegion.getField< contact::dispJump >();
      arrayView2d< real64 const > const traction      = subRegion.getField< contact::traction >();

      arrayView1d< real64 > const stateVariable_n = subRegion.getField< rateAndState::stateVariable_n >();
      arrayView2d< real64 > const slipVelocity_n  = subRegion.getField< rateAndState::slipVelocity_n >();
      arrayView2d< real64 > const deltaSlip_n     = subRegion.getField< rateAndState::deltaSlip >();
      arrayView2d< real64 > const dispJump_n      = subRegion.getField< contact::dispJump_n >();
      arrayView2d< real64 > const traction_n      = subRegion.getField< contact::traction_n >();

      forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
      {
        stateVariable_n[k]  = stateVariable[k];
        LvArray::tensorOps::copy< 2 >( deltaSlip_n[k], deltaSlip[k] );
        LvArray::tensorOps::copy< 2 >( slipVelocity_n[k], slipVelocity[k] );
        LvArray::tensorOps::copy< 3 >( dispJump_n[k], dispJump[k] );
        LvArray::tensorOps::copy< 3 >( traction_n[k], traction[k] );
      } );
    } );
  } );
}

real64 QuasiDynamicEQRK32::setNextDt( real64 const & currentDt, DomainPartition & domain )
{

  // Spring-slider shear traction computation
  forDiscretizationOnMeshTargets( domain.getMeshBodies(), [&]( string const &,
                                                               MeshLevel const & mesh,
                                                               arrayView1d< string const > const & regionNames )

  {
    mesh.getElemManager().forElementSubRegions< SurfaceElementSubRegion >( regionNames,
                                                                           [&]( localIndex const,
                                                                                SurfaceElementSubRegion const & subRegion )
    {
      arrayView2d< real64 const > const error = subRegion.getField< rateAndState::error >();

      RAJA::ReduceSum< parallelDeviceReduce, real64 > scaledl2ErrorSquared( 0.0 );
      integer const N = subRegion.size();
      forAll< parallelDevicePolicy<> >( subRegion.size(), [=] GEOS_HOST_DEVICE ( localIndex const k )
      {
        scaledl2ErrorSquared += LvArray::tensorOps::l2NormSquared< 3 >( error[k] );
      } );
      m_controller.errors[0] = LvArray::math::sqrt( MpiWrapper::sum( scaledl2ErrorSquared.get() / (3.0*N) ));
    } );
  } );

  // Compute update factor to currentDt using PID error controller + limiter
  real64 const dtFactor = m_controller.computeUpdateFactor( m_butcherTable.algHighOrder, m_butcherTable.algLowOrder );
  real64 const nextDt = dtFactor*currentDt;
  // Check if step was acceptable
  m_successfulStep = (dtFactor >= m_controller.acceptSafety) ? true : false;
  if( m_successfulStep )
  {
    m_controller.errors[2] = m_controller.errors[1];
    m_controller.errors[1] = m_controller.errors[0];
    GEOS_LOG_LEVEL_RANK_0( 1, GEOS_FMT( "Adaptive time step successful. The next dt will be {:.2e} s", nextDt ));
  }
  else
  {
    GEOS_LOG_LEVEL_RANK_0( 1, GEOS_FMT( "Adaptive time step failed. The next dt will be {:.2e} s", nextDt ));
  }

  return nextDt;
}

REGISTER_CATALOG_ENTRY( PhysicsSolverBase, QuasiDynamicEQRK32, string const &, dataRepository::Group * const )

} // namespace geos
