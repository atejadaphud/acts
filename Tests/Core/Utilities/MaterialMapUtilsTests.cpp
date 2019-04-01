// This file is part of the Acts project.
//
// Copyright (C) 2019 Acts project team
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// clang-format off
#define BOOST_TEST_MODULE bfield utils test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
// clang-format on

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/MaterialMapUtils.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/detail/Axis.hpp"
#include <limits>
#include <random>
#include <vector>
#include "Acts/Detector/TrackingGeometry.hpp"
#include "Acts/Detector/TrackingVolume.hpp"
#include "Acts/EventData/SingleCurvilinearTrackParameters.hpp"
#include "Acts/Extrapolator/Navigator.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Utilities/MaterialMapUtils.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StraightLineStepper.hpp"
#include "Acts/Propagator/detail/StandardAborters.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tools/CuboidVolumeBuilder.hpp"
#include "Acts/Tools/TrackingGeometryBuilder.hpp"
#include "Acts/Utilities/Definitions.hpp"
#include "Acts/Utilities/detail/Grid.hpp"

namespace bdata = boost::unit_test::data;

using Acts::VectorHelpers::perp;

namespace Acts {

using namespace detail;

namespace Test {

  /// @brief This function assigns all material points to the fith bin.
  ///
  /// @return 5
  Grid2D::index_t
  mapToZero2D(const Vector3D& /*unused*/, const Grid2D& /*unused*/)
  {
    return {{0, 0}};
  }

  /// @brief This function assigns all material points to the fith bin.
  ///
  /// @return 25
  Grid3D::index_t
  mapToZero3D(const Vector3D& /*unused*/, const Grid3D& /*unused*/)
  {
    return {{0, 0, 0}};
  }

  /// @brief This function assigns material to the 2D bin number that represents
  /// the local index of the first axis to the material point.
  ///
  /// @param [in] matPos Position of the material
  /// @param [in] grid Grid that is used for the look-up
  ///
  /// @return Local grid point with the closest distance to @p matPos along the
  /// first axis
  Grid2D::index_t
  mapToBin2D(const Vector3D& matPos, const Grid2D& grid)
  {
    double dist  = std::numeric_limits<double>::max();
    size_t index = 0;
    // Loop through all elements in the first axis
    for (size_t i = 0; i < grid.getNBins()[0]; i++) {
      // Search the closest distance - elements are ordered
      if (std::abs(grid.getUpperRightBinEdge({{i, 0}})[0] - matPos.x())
          < dist) {
        // Store distance and index
        dist  = std::abs(grid.getUpperRightBinEdge({{i, 0}})[0] - matPos.x());
        index = i;
      } else {  // Break if distance becomes larger
        break;
      }
    }
    return {{index, 0}};
  }

  /// @brief This function assigns material to the 3D bin number that represents
  /// the local index of the first axis to the material point.
  ///
  /// @param [in] matPos Position of the material
  /// @param [in] grid Grid that is used for the look-up
  ///
  /// @return Local grid point with the closest distance to @p matPos along the
  /// first axis
  Grid3D::index_t
  mapToBin3D(const Vector3D& matPos, const Grid3D& grid)
  {
    double dist  = std::numeric_limits<double>::max();
    size_t index = 0;
    // Loop through all elements in the first axis
    for (size_t i = 0; i < grid.getNBins()[0]; i++) {
      // Search the closest distance - elements are ordered
      if (std::abs(grid.getUpperRightBinEdge({{i, 0, 0}})[0] - matPos.x())
          < dist) {
        // Store distance and index
        dist = std::abs(grid.getUpperRightBinEdge({{i, 0, 0}})[0] - matPos.x());
        index = i;
      } else {  // Break if distance becomes larger
        break;
      }
    }
    return {{index, 0, 0}};
  }

  Grid3D::index_t
  mapMaterial3D(const Vector3D& matPos, const Grid3D& grid)
  {
    double dist   = std::numeric_limits<double>::max();
    size_t indexX = 0, indexY = 0, indexZ = 0;
    // Loop through all elements
    for (size_t i = 0; i < grid.getNBins()[0]; i++) {
      for (size_t j = 0; j < grid.getNBins()[1]; j++) {
        for (size_t k = 0; k < grid.getNBins()[2]; k++) {
          // Search the closest distance - elements are ordered
          double dX = grid.getUpperRightBinEdge({{i, j, k}})[0] - matPos.x();
          double dY = grid.getUpperRightBinEdge({{i, j, k}})[1] - matPos.y();
          double dZ = grid.getUpperRightBinEdge({{i, j, k}})[2] - matPos.z();

          if (std::sqrt(dX * dX + dY * dY + dZ * dZ) < dist) {
            // Store distance and index
            dist   = std::sqrt(dX * dX + dY * dY + dZ * dZ);
            indexX = i;
            indexY = j;
            indexZ = k;
          } else {  // Break if distance becomes larger
            break;
          }
        }
      }
    }
    return {{indexX, indexY, indexZ}};
  }

  struct MaterialCollector
  {
    struct this_result
    {
      std::vector<Material> matTrue;
      std::vector<Vector3D> position;
    };
    using result_type = this_result;

    template <typename propagator_state_t, typename stepper_t>
    void
    operator()(propagator_state_t& state,
               const stepper_t&    stepper,
               result_type&        result) const
    {
      if (state.navigation.currentVolume != nullptr) {
        result.matTrue.push_back(
            (state.navigation.currentVolume->material() != nullptr)
                ? *state.navigation.currentVolume->material()
                : Material());

        result.position.push_back(stepper.position(state.stepping));
      }
    }
  };

  /// @brief Various test cases of the VolumeMaterialMapper functions
  BOOST_AUTO_TEST_CASE(VolumeMaterialMapper_tests)
  {
    // Define some axes and grid points
    std::vector<double> axis1 = {0., 1.};
    std::vector<double> axis2 = {3., 4., 2.};
    std::vector<double> axis3 = {5., 6., 7.};

    //
    // Test block for createState
    //
    // Test that a 2D grid could be created
    Grid2D grid2d = createGrid(axis1, axis2);
    BOOST_CHECK_EQUAL(grid2d.getAxes().size(), 2);
    // Test the number of bins
    Grid2D::index_t nBins2d = grid2d.getNBins();
    BOOST_CHECK_EQUAL(nBins2d[0], axis1.size());
    BOOST_CHECK_EQUAL(nBins2d[1], axis2.size());
    // Test the limits
    Grid2D::point_t min2d = grid2d.getMin();
    CHECK_CLOSE_ABS(min2d[0], axis1[0], 1e-4);
    CHECK_CLOSE_REL(min2d[1], axis2[2], 1e-4);
    Grid2D::point_t max2d = grid2d.getMax();
    CHECK_CLOSE_REL(max2d[0], axis1[1] + 1, 1e-4);
    CHECK_CLOSE_REL(max2d[1], axis2[1] + 1, 1e-4);

    // And again for 3 axes
    Grid3D grid3d = createGrid(axis1, axis2, axis3);
    BOOST_CHECK_EQUAL(grid3d.getAxes().size(), 3);
    // Test the number of bins
    Grid3D::index_t nBins3d = grid3d.getNBins();
    BOOST_CHECK_EQUAL(nBins3d[0], axis1.size());
    BOOST_CHECK_EQUAL(nBins3d[1], axis2.size());
    BOOST_CHECK_EQUAL(nBins3d[2], axis3.size());
    // Test the limits
    Grid3D::point_t min3d = grid3d.getMin();
    CHECK_CLOSE_ABS(min3d[0], axis1[0], 1e-4);
    CHECK_CLOSE_REL(min3d[1], axis2[2], 1e-4);
    CHECK_CLOSE_REL(min3d[2], axis3[0], 1e-4);
    Grid3D::point_t max3d = grid3d.getMax();
    CHECK_CLOSE_REL(max3d[0], axis1[1] + 1, 1e-4);
    CHECK_CLOSE_REL(max3d[1], axis2[1] + 1, 1e-4);
    CHECK_CLOSE_REL(max3d[2], axis3[2] + 1, 1e-4);

    //
    // Test block for mapMaterialPoints in 2D
    //
    Material mat1(1., 2., 3., 4., 5.);
    std::vector<std::pair<Material, Vector3D>> matRecord;
    matRecord.push_back(std::make_pair(mat1, Vector3D(0., 0., 0.)));

    // Check if material can be assigned by the function
    MaterialGrid2D mgrid2d = mapMaterialPoints(grid2d, matRecord, mapToZero2D);
    BOOST_CHECK_EQUAL(mgrid2d.getNBins()[0], nBins2d[0]);
    BOOST_CHECK_EQUAL(mgrid2d.getNBins()[1], nBins2d[1]);
    CHECK_CLOSE_ABS(mgrid2d.getMin()[0], min2d[0], 1e-4);
    CHECK_CLOSE_REL(mgrid2d.getMin()[1], min2d[1], 1e-4);
    CHECK_CLOSE_REL(mgrid2d.getMax()[0], max2d[0], 1e-4);
    CHECK_CLOSE_REL(mgrid2d.getMax()[1], max2d[1], 1e-4);

    CHECK_CLOSE_REL(grid2d.at((size_t)0).average(), mat1, 1e-4);
    CHECK_CLOSE_REL(
        mgrid2d.at((size_t)0), mat1.decomposeIntoClassificationNumbers(), 1e-4);

    // Check that it was only assigned to a single bin
    for (size_t i = 1; i < grid2d.size(); i++) {
      BOOST_CHECK_EQUAL(
          grid2d.at(i).average().decomposeIntoClassificationNumbers(),
          Material().decomposeIntoClassificationNumbers());
      BOOST_CHECK_EQUAL(mgrid2d.at(i),
                        Material().decomposeIntoClassificationNumbers());
    }

    // Check if the assignment to a custom bin is possible
    Material mat2(6., 7., 8., 9., 10.);
    matRecord.clear();
    matRecord.push_back(std::make_pair(mat2, Vector3D(0.4, 0., 0.)));
    matRecord.push_back(std::make_pair(mat2, Vector3D(0.6, 0., 0.)));
    mgrid2d = mapMaterialPoints(grid2d, matRecord, mapToBin2D);

    // Check that the first element now has both materials
    CHECK_CLOSE_REL(grid2d.at((size_t)0).average().X0(),
                    0.5 * (mat1.X0() + mat2.X0()),
                    1e-4);
    CHECK_CLOSE_REL(grid2d.at((size_t)0).average().L0(),
                    0.5 * (mat1.L0() + mat2.L0()),
                    1e-4);
    CHECK_CLOSE_REL(
        grid2d.at((size_t)0).average().A(), 0.5 * (mat1.A() + mat2.A()), 1e-4);
    CHECK_CLOSE_REL(
        grid2d.at((size_t)0).average().Z(), 0.5 * (mat1.Z() + mat2.Z()), 1e-4);
    CHECK_CLOSE_REL(grid2d.at((size_t)0).average().rho(),
                    0.5 * (mat1.rho() + mat2.rho()),
                    1e-4);
    CHECK_CLOSE_REL(
        grid2d.at((size_t)0).average().decomposeIntoClassificationNumbers(),
        mgrid2d.at((size_t)0),
        1e-4);
    // Check that the second element has a single material
    CHECK_CLOSE_REL(grid2d.at((size_t)5).average().X0(), mat2.X0(), 1e-4);
    CHECK_CLOSE_REL(grid2d.at((size_t)5).average().L0(), mat2.L0(), 1e-4);
    CHECK_CLOSE_REL(grid2d.at((size_t)5).average().A(), mat2.A(), 1e-4);
    CHECK_CLOSE_REL(grid2d.at((size_t)5).average().Z(), mat2.Z(), 1e-4);
    CHECK_CLOSE_REL(grid2d.at((size_t)5).average().rho(), mat2.rho(), 1e-4);

    // Check that nothing was assigned to the other elements
    for (size_t i = 1; i < grid2d.size(); i++) {
      if (i == 5) {
        continue;
      }
      BOOST_CHECK_EQUAL(
          grid2d.at(i).average().decomposeIntoClassificationNumbers(),
          Material().decomposeIntoClassificationNumbers());
    }

    //
    // Test block for mapMaterialPoints in 3D
    //
    matRecord.clear();
    matRecord.push_back(std::make_pair(mat1, Vector3D(0., 0., 0.)));

    // Check if material can be assigned by the function
    MaterialGrid3D mgrid3d = mapMaterialPoints(grid3d, matRecord, mapToZero3D);
    BOOST_CHECK_EQUAL(mgrid3d.getNBins()[0], nBins3d[0]);
    BOOST_CHECK_EQUAL(mgrid3d.getNBins()[1], nBins3d[1]);
    CHECK_CLOSE_ABS(mgrid3d.getMin()[0], min3d[0], 1e-4);
    CHECK_CLOSE_REL(mgrid3d.getMin()[1], min3d[1], 1e-4);
    CHECK_CLOSE_REL(mgrid3d.getMax()[0], max3d[0], 1e-4);
    CHECK_CLOSE_REL(mgrid3d.getMax()[1], max3d[1], 1e-4);

    CHECK_CLOSE_REL(grid3d.at((size_t)0).average(), mat1, 1e-4);
    CHECK_CLOSE_REL(
        mgrid3d.at((size_t)0), mat1.decomposeIntoClassificationNumbers(), 1e-4);

    // Check that it was only assigned to a single bin
    for (size_t i = 1; i < grid3d.size(); i++) {
      BOOST_CHECK_EQUAL(
          grid3d.at(i).average().decomposeIntoClassificationNumbers(),
          Material().decomposeIntoClassificationNumbers());
      BOOST_CHECK_EQUAL(mgrid3d.at(i),
                        Material().decomposeIntoClassificationNumbers());
    }

    // Check if the assignment to a custom bin is possible
    matRecord.clear();
    matRecord.push_back(std::make_pair(mat2, Vector3D(0.4, 0., 0.)));
    matRecord.push_back(std::make_pair(mat2, Vector3D(0.6, 0., 0.)));
    mgrid3d = mapMaterialPoints(grid3d, matRecord, mapToBin3D);

    // Check that the first element now has both materials
    CHECK_CLOSE_REL(grid3d.at((size_t)0).average().X0(),
                    0.5 * (mat1.X0() + mat2.X0()),
                    1e-4);
    CHECK_CLOSE_REL(grid3d.at((size_t)0).average().L0(),
                    0.5 * (mat1.L0() + mat2.L0()),
                    1e-4);
    CHECK_CLOSE_REL(
        grid3d.at((size_t)0).average().A(), 0.5 * (mat1.A() + mat2.A()), 1e-4);
    CHECK_CLOSE_REL(
        grid3d.at((size_t)0).average().Z(), 0.5 * (mat1.Z() + mat2.Z()), 1e-4);
    CHECK_CLOSE_REL(grid3d.at((size_t)0).average().rho(),
                    0.5 * (mat1.rho() + mat2.rho()),
                    1e-4);
    CHECK_CLOSE_REL(
        grid3d.at((size_t)0).average().decomposeIntoClassificationNumbers(),
        mgrid3d.at((size_t)0),
        1e-4);
    // Check that the second element has a single material
    CHECK_CLOSE_REL(grid3d.at((size_t)25).average().X0(), mat2.X0(), 1e-4);
    CHECK_CLOSE_REL(grid3d.at((size_t)25).average().L0(), mat2.L0(), 1e-4);
    CHECK_CLOSE_REL(grid3d.at((size_t)25).average().A(), mat2.A(), 1e-4);
    CHECK_CLOSE_REL(grid3d.at((size_t)25).average().Z(), mat2.Z(), 1e-4);
    CHECK_CLOSE_REL(grid3d.at((size_t)25).average().rho(), mat2.rho(), 1e-4);

    // Check that nothing was assigned to the other elements
    for (size_t i = 1; i < grid3d.size(); i++) {
      if (i == 25) {
        continue;
      }
      BOOST_CHECK_EQUAL(
          grid3d.at(i).average().decomposeIntoClassificationNumbers(),
          Material().decomposeIntoClassificationNumbers());
    }

    //
    // Test the full production chain in 2D
    //
    matRecord.clear();
    matRecord.push_back(std::make_pair(mat1, Vector3D(0., 0., 0.)));
    matRecord.push_back(std::make_pair(mat2, Vector3D(0.4, 0., 0.)));
    matRecord.push_back(std::make_pair(mat2, Vector3D(0.6, 0., 0.)));
    auto           tmpGrid2D = createGrid(axis1, axis2);
    MaterialGrid2D mgrid2dStepChain
        = mapMaterialPoints(tmpGrid2D, matRecord, mapToBin2D);
    MaterialGrid2D mgrid2dFullChain
        = createMaterialGrid(axis1, axis2, matRecord, mapToBin2D);

    // Test sizes
    BOOST_CHECK_EQUAL(mgrid2dFullChain.size(), mgrid2dStepChain.size());
    for (size_t index = 0; index < mgrid2dFullChain.size(); index++) {
      // Both should contain the same data
      BOOST_CHECK_EQUAL(mgrid2dFullChain.at(index), mgrid2dStepChain.at(index));
    }

    //
    // Test the full production chain in 3D
    //
    auto           tmpGrid3D = createGrid(axis1, axis2, axis3);
    MaterialGrid3D mgrid3dStepChain
        = mapMaterialPoints(tmpGrid3D, matRecord, mapToBin3D);
    MaterialGrid3D mgrid3dFullChain
        = createMaterialGrid(axis1, axis2, axis3, matRecord, mapToBin3D);

    // Test sizes
    BOOST_CHECK_EQUAL(mgrid3dFullChain.size(), mgrid3dStepChain.size());
    for (size_t index = 0; index < mgrid3dFullChain.size(); index++) {
      // Both should contain the same data
      BOOST_CHECK_EQUAL(mgrid3dFullChain.at(index), mgrid3dStepChain.at(index));
    }
  }

  /// @brief Test case for comparison between the mapped material and the
  /// associated material by propagation
  BOOST_AUTO_TEST_CASE(VolumeMaterialMapper_comparison_tests)
  {
    // Build a vacuum volume
    CuboidVolumeBuilder::VolumeConfig vCfg1;
    vCfg1.position = Vector3D(0.5 * units::_m, 0., 0.);
    vCfg1.length   = Vector3D(1. * units::_m, 1. * units::_m, 1. * units::_m);
    vCfg1.name     = "Vacuum volume";
    vCfg1.material
        = std::make_shared<const Material>(352.8, 407., 9.012, 4., 1.848e-3);

    // Build a material volume
    CuboidVolumeBuilder::VolumeConfig vCfg2;
    vCfg2.position = Vector3D(1.5 * units::_m, 0., 0.);
    vCfg2.length   = Vector3D(1. * units::_m, 1. * units::_m, 1. * units::_m);
    vCfg2.name     = "First material volume";
    vCfg2.material
        = std::make_shared<const Material>(95.7, 465.2, 28.03, 14., 2.32e-3);

    // Build another material volume with different material
    CuboidVolumeBuilder::VolumeConfig vCfg3;
    vCfg3.position = Vector3D(2.5 * units::_m, 0., 0.);
    vCfg3.length   = Vector3D(1. * units::_m, 1. * units::_m, 1. * units::_m);
    vCfg3.name     = "Second material volume";
    vCfg3.material
        = std::make_shared<const Material>(352.8, 407., 9.012, 4., 1.848e-3);

    // Configure world
    CuboidVolumeBuilder::Config cfg;
    cfg.position  = Vector3D(1.5 * units::_m, 0., 0.);
    cfg.length    = Vector3D(3. * units::_m, 1. * units::_m, 1. * units::_m);
    cfg.volumeCfg = {vCfg1, vCfg2, vCfg3};

    GeometryContext gc;

    // Build a detector
    CuboidVolumeBuilder             cvb(cfg);
    TrackingGeometryBuilder::Config tgbCfg;
    //~ tgbCfg.trackingVolumeBuilders.push_back(
    //~ std::make_shared<const CuboidVolumeBuilder>(cvb));
    tgbCfg.trackingVolumeBuilders.push_back(
        [=](const auto& context, const auto& inner, const auto&) {
          return cvb.trackingVolume(context, inner, nullptr);
        });
    TrackingGeometryBuilder                 tgb(tgbCfg);
    std::unique_ptr<const TrackingGeometry> detector = tgb.trackingGeometry(gc);

    // Set up the grid axes
    unsigned int        nGridPoints = 9;
    std::vector<double> xAxis(nGridPoints + 1,
                              3. * units::_m / (double)nGridPoints);
    std::vector<double> yAxis(nGridPoints + 1,
                              1. * units::_m / (double)nGridPoints);
    std::vector<double> zAxis(nGridPoints + 1,
                              1. * units::_m / (double)nGridPoints);
    for (unsigned int i = 0; i <= nGridPoints; i++) {
      xAxis[i] *= i;
      yAxis[i] *= i;
      zAxis[i] *= i;
    }

    // Set up a random engine for sampling material
    std::random_device               rd;
    std::mt19937                     gen(rd());
    std::uniform_real_distribution<> disX(0., 3. * units::_m);
    std::uniform_real_distribution<> disYZ(-0.5 * units::_m, 0.5 * units::_m);

    // Sample the Material in the detector
    RecordedMaterial matRecord;
    for (unsigned int i = 0; i < 1e4; i++) {
      Vector3D pos(disX(gen), disYZ(gen), disYZ(gen));
      Material tv
          = (detector->lowestTrackingVolume(gc, pos)->material() != nullptr)
          ? *(detector->lowestTrackingVolume(gc, pos)->material())
          : Material();
      matRecord.push_back(std::make_pair(tv, pos));
    }

    // Build the material grid
    MaterialGrid3D grid
        = createMaterialGrid(xAxis, yAxis, zAxis, matRecord, mapMaterial3D);

    // Construct a simple propagation through the detector
    StraightLineStepper sls;
    Navigator           nav(std::move(detector));
    Propagator<StraightLineStepper, Navigator> prop(sls, nav);

    // Set some start parameters
    Vector3D pos(0., 0., 0.);
    Vector3D mom(1. * units::_GeV, 0., 0.);
    SingleCurvilinearTrackParameters<NeutralPolicy> sctp(nullptr, pos, mom);

    MagneticFieldContext mc;

    // Launch propagation and gather result
    PropagatorOptions<ActionList<MaterialCollector>,
                      AbortList<detail::EndOfWorldReached>>
        po(gc, mc);
    po.maxStepSize     = 1. * units::_mm;
    po.maxSteps        = 1e6;
    const auto& result = prop.propagate(sctp, po).value();
    const MaterialCollector::this_result& stepResult
        = result.get<typename MaterialCollector::result_type>();

    // Collect the material as given by the grid and test it
    std::vector<Material> matGrid;
    double                gridX0 = 0., gridL0 = 0., trueX0 = 0., trueL0 = 0.;
    for (unsigned int i = 0; i < stepResult.position.size(); i++) {
      matGrid.push_back(grid.at(stepResult.position[i]));
      gridX0 += matGrid[i].X0();
      gridL0 += matGrid[i].L0();
      trueX0 += stepResult.matTrue[i].X0();
      trueL0 += stepResult.matTrue[i].L0();
    }
    CHECK_CLOSE_REL(gridX0, trueX0, 1e-1);
    CHECK_CLOSE_REL(gridL0, trueL0, 1e-1);
  }

  BOOST_AUTO_TEST_CASE(materialmap_creation)
  {
    // Create grid values
    std::vector<double> rPos = {0., 1., 2.};
    std::vector<double> xPos = {0., 1., 2.};
    std::vector<double> yPos = {0., 1., 2.};
    std::vector<double> zPos = {0., 1., 2.};

    // Create material association in rz
    std::vector<Material> material_rz;
    for (int i = 0; i < 9; i++) {
      material_rz.push_back(Material(i, i, i, i, i));
    }

    auto localToGlobalBin_rz
        = [](std::array<size_t, 2> binsRZ, std::array<size_t, 2> nBinsRZ) {
            return (binsRZ.at(1) * nBinsRZ.at(0) + binsRZ.at(0));
          };
    // Create material mapper in rz
    auto mapper_rz
        = materialMapperRZ(localToGlobalBin_rz, rPos, zPos, material_rz);
    // check number of bins, minima & maxima
    std::vector<size_t> nBins_rz  = {rPos.size(), zPos.size()};
    std::vector<double> minima_rz = {0., 0.};
    std::vector<double> maxima_rz = {3., 3.};
    BOOST_CHECK(mapper_rz.getNBins() == nBins_rz);
    // Check minimum (should be first value because bin values are always
    // assigned to the left boundary)
    BOOST_CHECK(mapper_rz.getMin() == minima_rz);
    // Check maximum (should be last value + 1 step because bin values are
    // always assigned to the left boundary)
    BOOST_CHECK(mapper_rz.getMax() == maxima_rz);

    // Create map in xyz
    std::vector<Material> material_xyz;
    for (int i = 0; i < 27; i++) {
      material_xyz.push_back(Material(i, i, i, i, i));
    }

    auto localToGlobalBin_xyz
        = [](std::array<size_t, 3> binsXYZ, std::array<size_t, 3> nBinsXYZ) {
            return (binsXYZ.at(0) * (nBinsXYZ.at(1) * nBinsXYZ.at(2))
                    + binsXYZ.at(1) * nBinsXYZ.at(2)
                    + binsXYZ.at(2));
          };

    // Create material mapper in xyz
    auto mapper_xyz = materialMapperXYZ(
        localToGlobalBin_xyz, xPos, yPos, zPos, material_xyz);
    // Check number of bins, minima & maxima
    std::vector<size_t> nBins_xyz  = {xPos.size(), yPos.size(), zPos.size()};
    std::vector<double> minima_xyz = {0., 0., 0.};
    std::vector<double> maxima_xyz = {3., 3., 3.};
    BOOST_CHECK(mapper_xyz.getNBins() == nBins_xyz);
    // Check minimum (should be first value because bin values are always
    // assigned to the left boundary)
    BOOST_CHECK(mapper_xyz.getMin() == minima_xyz);
    // Check maximum (should be last value + 1 step because bin values are
    // always assigned to the left boundary)
    BOOST_CHECK(mapper_xyz.getMax() == maxima_xyz);

    // Check if filled value is expected value in rz
    Vector3D pos0_rz(0., 0., 0.);
    Vector3D pos1_rz(1., 0., 1.);
    Vector3D pos2_rz(0., 2., 2.);
    auto     value0_rz = mapper_rz.getMaterial(pos0_rz);
    auto     value1_rz = mapper_rz.getMaterial(pos1_rz);
    auto     value2_rz = mapper_rz.getMaterial(pos2_rz);
    // Calculate what the value should be at this point
    Material mat0_rz = material_rz.at(
        localToGlobalBin_rz({{0, 0}}, {{rPos.size(), zPos.size()}}));
    Material mat1_rz = material_rz.at(
        localToGlobalBin_rz({{1, 1}}, {{rPos.size(), zPos.size()}}));
    Material mat2_rz = material_rz.at(
        localToGlobalBin_rz({{2, 2}}, {{rPos.size(), zPos.size()}}));

    // Check the value
    // in rz case material is phi symmetric (check radius)
    CHECK_CLOSE_ABS(value0_rz.decomposeIntoClassificationNumbers(),
                    mat0_rz.decomposeIntoClassificationNumbers(),
                    1e-9);
    CHECK_CLOSE_ABS(value1_rz.decomposeIntoClassificationNumbers(),
                    mat1_rz.decomposeIntoClassificationNumbers(),
                    1e-9);
    CHECK_CLOSE_ABS(value2_rz.decomposeIntoClassificationNumbers(),
                    mat2_rz.decomposeIntoClassificationNumbers(),
                    1e-9);

    // Check if filled value is expected value in xyz
    Vector3D pos0_xyz(0., 0., 0.);
    Vector3D pos1_xyz(1., 1., 1.);
    Vector3D pos2_xyz(2., 2., 2.);
    auto     value0_xyz = mapper_xyz.getMaterial(pos0_xyz);
    auto     value1_xyz = mapper_xyz.getMaterial(pos1_xyz);
    auto     value2_xyz = mapper_xyz.getMaterial(pos2_xyz);
    // Calculate what the value should be at this point
    Material mat0_xyz = material_xyz.at(localToGlobalBin_xyz(
        {{0, 0, 0}}, {{xPos.size(), yPos.size(), zPos.size()}}));
    Material mat1_xyz = material_xyz.at(localToGlobalBin_xyz(
        {{1, 1, 1}}, {{xPos.size(), yPos.size(), zPos.size()}}));
    Material mat2_xyz = material_xyz.at(localToGlobalBin_xyz(
        {{2, 2, 2}}, {{xPos.size(), yPos.size(), zPos.size()}}));

    // Check the value
    // in xyz case material is phi symmetric (check radius)
    CHECK_CLOSE_ABS(value0_xyz.decomposeIntoClassificationNumbers(),
                    mat0_xyz.decomposeIntoClassificationNumbers(),
                    1e-9);
    CHECK_CLOSE_ABS(value1_xyz.decomposeIntoClassificationNumbers(),
                    mat1_xyz.decomposeIntoClassificationNumbers(),
                    1e-9);
    CHECK_CLOSE_ABS(value2_xyz.decomposeIntoClassificationNumbers(),
                    mat2_xyz.decomposeIntoClassificationNumbers(),
                    1e-9);
  }
}
}
