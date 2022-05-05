// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of NTESS nor the names of its contributors
//       may be used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#include <gtest/gtest.h>
#include <stk_mesh/base/Ngp.hpp>
#include <stk_mesh/base/NgpMesh.hpp>
#include <stk_mesh/base/NgpField.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/FieldBase.hpp>
#include <stk_mesh/base/FieldDataManager.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/GetNgpField.hpp>
#include <stk_mesh/base/GetNgpMesh.hpp>
#include <stk_mesh/base/MeshBuilder.hpp>
#include <stk_topology/topology.hpp>
#include <stk_mesh/base/CoordinateSystems.hpp>
#include <stk_unit_test_utils/MeshFixture.hpp>
#include <stk_unit_test_utils/GetMeshSpec.hpp>
#include <stk_performance_tests/stk_mesh/timer.hpp>
#include <stk_io/FillMesh.hpp>
#include <vector>
#include <string>

namespace
{

double initial_value[3] = {-1, 2, -0.3};

class NgpFieldAccessPerformance : public stk::unit_test_util::simple_fields::MeshFixture
{
public:
  using DoubleVecField = stk::mesh::Field<double>;

  NgpFieldAccessPerformance()
    : timer(get_comm()),
      m_fieldDataManager(nullptr)
  { }

  virtual ~NgpFieldAccessPerformance() {
    delete m_fieldDataManager;
    m_fieldDataManager = nullptr;
  }

  void setup_empty_mesh_with_field_data_manager(stk::mesh::BulkData::AutomaticAuraOption auraOption,
                                                stk::mesh::FieldDataManager & fieldDataManager,
                                                unsigned bucketCapacity = stk::mesh::impl::BucketRepository::default_bucket_capacity)
  {
    stk::mesh::MeshBuilder builder(communicator);
    builder.set_spatial_dimension(m_spatialDim);
    builder.set_entity_rank_names(m_entityRankNames);
    builder.set_aura_option(auraOption);
    builder.set_field_data_manager(&fieldDataManager);
    builder.set_bucket_capacity(bucketCapacity);

    bulkData = builder.create();
  }

  DoubleVecField * createNodalVectorField(const std::string &field_name)
  {
    DoubleVecField &field = get_meta().declare_field<double>(stk::topology::NODE_RANK, field_name);
    stk::mesh::put_field_on_entire_mesh_with_initial_value(field, initial_value);
    return &field;
  }

  void createNodalVectorFields()
  {
    dispField  = createNodalVectorField("disp");
    velField   = createNodalVectorField("vel");
    accField   = createNodalVectorField("acc");
    forceField = createNodalVectorField("force");
  }

  void testVectorFieldSum()
  {
    stk::mesh::NgpField<double> & ngpDispField  = stk::mesh::get_updated_ngp_field<double>(*dispField);
    stk::mesh::NgpField<double> & ngpVelField   = stk::mesh::get_updated_ngp_field<double>(*velField);
    stk::mesh::NgpField<double> & ngpAccField   = stk::mesh::get_updated_ngp_field<double>(*accField);
    stk::mesh::NgpField<double> & ngpForceField = stk::mesh::get_updated_ngp_field<double>(*forceField);
    stk::mesh::NgpMesh & ngpMesh = stk::mesh::get_updated_ngp_mesh(get_bulk());
    const int spatialDimension = static_cast<int>(get_meta().spatial_dimension());

    const int numRuns = 500000;

    timer.start_timing();
    for (int run = 0; run < numRuns; ++run) {
      stk::mesh::for_each_entity_run(ngpMesh, stk::topology::NODE_RANK, get_meta().locally_owned_part(),
                                     KOKKOS_LAMBDA(const stk::mesh::FastMeshIndex & index)
                                     {
                                       for (int component = 0; component < spatialDimension; ++component) {
                                         ngpForceField(index, component) = alpha * ngpDispField(index, component) +
                                                                           beta * ngpVelField(index, component) +
                                                                           gamma * ngpAccField(index, component);
                                       }
                                     });
    }
    timer.update_timing();
    timer.print_timing(numRuns);

    ngpForceField.modify_on_device();
  }

  void testHostVectorFieldSum()
  {
    stk::mesh::get_updated_ngp_field<double>(*dispField);
    stk::mesh::get_updated_ngp_field<double>(*velField);
    stk::mesh::get_updated_ngp_field<double>(*accField);
    stk::mesh::get_updated_ngp_field<double>(*forceField);
    const int spatialDimension = static_cast<int>(get_meta().spatial_dimension());

    const int numRuns = 5000;

    timer.start_timing();
    for (int run = 0; run < numRuns; ++run) {
      const stk::mesh::BucketVector & buckets = get_bulk().get_buckets(stk::topology::NODE_RANK, get_meta().locally_owned_part());
      for (const stk::mesh::Bucket * bucket : buckets) {
        for (const stk::mesh::Entity & entity : *bucket) {
          const double * dispFieldData = stk::mesh::field_data(*dispField, entity);
          const double *  velFieldData = stk::mesh::field_data(*velField, entity);
          const double *  accFieldData = stk::mesh::field_data(*accField, entity);
          double * forceFieldData = stk::mesh::field_data(*forceField, entity);
          for (int component = 0; component < spatialDimension; ++component) {
            forceFieldData[component] = alpha * dispFieldData[component] +
                                        beta *   velFieldData[component] +
                                        gamma *  accFieldData[component];
          }
        }
      }
    }
    timer.update_timing();
    timer.print_timing(numRuns);
  }

  void testPureHostVectorFieldSum()
  {
    const int spatialDimension = static_cast<int>(get_meta().spatial_dimension());

    const int numRuns = 5000;

    timer.start_timing();
    for (int run = 0; run < numRuns; ++run) {
      const stk::mesh::BucketVector & buckets = get_bulk().get_buckets(stk::topology::NODE_RANK, get_meta().locally_owned_part());
      for (const stk::mesh::Bucket * bucket : buckets) {
        for (const stk::mesh::Entity & entity : *bucket) {
          const double * dispFieldData = stk::mesh::field_data(*dispField, entity);
          const double *  velFieldData = stk::mesh::field_data(*velField, entity);
          const double *  accFieldData = stk::mesh::field_data(*accField, entity);
          double * forceFieldData = stk::mesh::field_data(*forceField, entity);
          for (int component = 0; component < spatialDimension; ++component) {
            forceFieldData[component] = alpha * dispFieldData[component] +
                                        beta *   velFieldData[component] +
                                        gamma *  accFieldData[component];
          }
        }
      }
    }
    timer.update_timing();
    timer.print_timing(numRuns);
  }

  void checkResult()
  {
    stk::mesh::NgpField<double> & ngpForceField = stk::mesh::get_updated_ngp_field<double>(*forceField);
    ngpForceField.sync_to_host();
    const int spatialDimension = static_cast<int>(get_meta().spatial_dimension());

    std::vector<double> expectedValues(spatialDimension);
    for (int i = 0; i < spatialDimension; ++i) {
      expectedValues[i] = alpha * initial_value[i] + beta * initial_value[i] + gamma * initial_value[i];
    }

    const stk::mesh::BucketVector & buckets = get_bulk().get_buckets(stk::topology::NODE_RANK, get_meta().locally_owned_part());
    for (stk::mesh::Bucket * bucket : buckets) {
      for (const stk::mesh::Entity & node : *bucket) {
        const double * forceFieldData = stk::mesh::field_data(*forceField, node);
        for (int component = 0; component < spatialDimension; ++component) {
          EXPECT_DOUBLE_EQ(forceFieldData[component], expectedValues[component]);
        }
      }
    }
  }

  void checkHostResult()
  {
    const int spatialDimension = static_cast<int>(get_meta().spatial_dimension());

    std::vector<double> expectedValues(spatialDimension);
    for (int i = 0; i < spatialDimension; ++i) {
      expectedValues[i] = alpha * initial_value[i] + beta * initial_value[i] + gamma * initial_value[i];
    }

    const stk::mesh::BucketVector & buckets = get_bulk().get_buckets(stk::topology::NODE_RANK, get_meta().locally_owned_part());
    for (const stk::mesh::Bucket * bucket : buckets) {
      for (const stk::mesh::Entity & entity : *bucket) {
        const double * forceFieldData = stk::mesh::field_data(*forceField, entity);
        for (int component = 0; component < spatialDimension; ++component) {
          EXPECT_DOUBLE_EQ(forceFieldData[component], expectedValues[component]);
        }
      }
    }
  }

  static constexpr double alpha = -1.4;
  static constexpr double beta = 0.3333333;
  static constexpr double gamma = 3.14159;

  stk::performance_tests::Timer timer;
  stk::mesh::FieldDataManager * m_fieldDataManager;

  DoubleVecField * dispField;
  DoubleVecField * velField;
  DoubleVecField * accField;
  DoubleVecField * forceField;
};

TEST_F(NgpFieldAccessPerformance, pureHost_vectorSum_DefaultFieldDataManager)
{
  if (get_parallel_size() != 1) return;

  unsigned numElemsPerDim = 100;
  const int weKnowThereAreFiveRanks = 5;
  m_fieldDataManager = new stk::mesh::DefaultFieldDataManager(weKnowThereAreFiveRanks);

  setup_empty_mesh_with_field_data_manager(stk::mesh::BulkData::NO_AUTO_AURA, *m_fieldDataManager);
  createNodalVectorFields();
  stk::io::fill_mesh(stk::unit_test_util::simple_fields::get_mesh_spec(numElemsPerDim), *bulkData);

  testPureHostVectorFieldSum();
  checkHostResult();
}

TEST_F(NgpFieldAccessPerformance, host_vectorSum_DefaultFieldDataManager)
{
  if (get_parallel_size() != 1) return;

  unsigned numElemsPerDim = 100;
  const int weKnowThereAreFiveRanks = 5;
  m_fieldDataManager = new stk::mesh::DefaultFieldDataManager(weKnowThereAreFiveRanks);

  setup_empty_mesh_with_field_data_manager(stk::mesh::BulkData::NO_AUTO_AURA, *m_fieldDataManager);
  createNodalVectorFields();
  stk::io::fill_mesh(stk::unit_test_util::simple_fields::get_mesh_spec(numElemsPerDim), *bulkData);

  testHostVectorFieldSum();
  checkHostResult();
}

TEST_F(NgpFieldAccessPerformance, vectorSum_DefaultFieldDataManager)
{
  if (get_parallel_size() != 1) return;

  unsigned numElemsPerDim = 100;
  const int weKnowThereAreFiveRanks = 5;
  m_fieldDataManager = new stk::mesh::DefaultFieldDataManager(weKnowThereAreFiveRanks);

  setup_empty_mesh_with_field_data_manager(stk::mesh::BulkData::NO_AUTO_AURA, *m_fieldDataManager);
  createNodalVectorFields();
  stk::io::fill_mesh(stk::unit_test_util::simple_fields::get_mesh_spec(numElemsPerDim), *bulkData);

  testVectorFieldSum();
  checkResult();
}

TEST_F(NgpFieldAccessPerformance, vectorSum_ContiguousFieldDataManager)
{
  if (get_parallel_size() != 1) return;

  unsigned numElemsPerDim = 100;
  m_fieldDataManager = new stk::mesh::ContiguousFieldDataManager;

  setup_empty_mesh_with_field_data_manager(stk::mesh::BulkData::NO_AUTO_AURA, *m_fieldDataManager);
  createNodalVectorFields();
  stk::io::fill_mesh(stk::unit_test_util::simple_fields::get_mesh_spec(numElemsPerDim), *bulkData);

  testVectorFieldSum();
  checkResult();
}





}
