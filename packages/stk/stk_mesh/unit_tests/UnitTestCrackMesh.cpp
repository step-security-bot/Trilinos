/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <stdexcept>

#include <stk_util/unit_test_support/stk_utest_macros.hpp>
#include <unit_tests/UnitTestMesh.hpp>
#include <unit_tests/UnitTestBoxFixture.hpp>
#include <unit_tests/UnitTestBoxMeshFixture.hpp>

namespace {

}

//----------------------------------------------------------------------------

STKUNIT_UNIT_TEST ( UnitTestCrackMesh , VerifyDestroy )
{
  stk::ParallelMachine pm = MPI_COMM_WORLD ;
  const unsigned p_rank = stk::parallel_machine_rank( pm );

  const unsigned nx = 3 , ny = 3 , nz = 3 ;

  for ( unsigned iy = 0 ; iy < ny ; ++iy ) {
  for ( unsigned ix = 0 ; ix < nx ; ++ix ) {
    QuadFixture fixture( pm , nx , ny );

    fixture.m_bulk_data.modification_begin();

    stk::mesh::Entity * elem = fixture.elem( ix , iy );

    if ( elem && p_rank == elem->owner_rank() ) {
      stk::mesh::Entity * tmp = elem ;
      fixture.m_bulk_data.destroy_entity( tmp );
    }

    fixture.m_bulk_data.modification_end();

    if ( elem ) {
      STKUNIT_EXPECT_TRUE ( elem->log_query() == stk::mesh::EntityLogDeleted );
    }
  }
  }

  for ( unsigned iz = 0 ; iz < nz ; ++iz ) {
  for ( unsigned iy = 0 ; iy < ny ; ++iy ) {
  for ( unsigned ix = 0 ; ix < nx ; ++ix ) {
    BoxFixture fixture( pm , nx , ny , nz );

    fixture.m_bulk_data.modification_begin();

    stk::mesh::Entity * elem = fixture.elem( ix , iy , iz );

    if ( elem && p_rank == elem->owner_rank() ) {
      stk::mesh::Entity * tmp = elem ;
      fixture.m_bulk_data.destroy_entity( tmp );
    }

    fixture.m_bulk_data.modification_end();

    if ( elem ) {
      STKUNIT_EXPECT_TRUE ( elem->log_query() == stk::mesh::EntityLogDeleted );
    }
  }
  }
  }
}

//----------------------------------------------------------------------------

STKUNIT_UNIT_TEST ( UnitTestCrackMesh , verifyBoxGhosting )
{
  BoxMeshFixture fixture( MPI_COMM_WORLD );

  stk::mesh::BulkData & mesh = fixture.m_bulk_data;

  fixture.fill_mesh();

  stk::mesh::Entity * const old_node = fixture.m_nodes[0][0][1];

  stk::mesh::Entity * const right_element = fixture.m_elems[0][0][1];

  unsigned right_ordinal = 0;
  unsigned new_node_id = 28;

  if ( old_node && right_element ) {
    stk::mesh::PairIterRelation rel = old_node->relations();

    for (; rel.first != rel.second; ++rel) {
      if ( (rel.first->entity()) == right_element) {
        right_ordinal = rel.first->identifier();
      }
    }
  }

  mesh.modification_begin();

  //only crack the mesh if I own the element
  if ( right_element &&
       right_element->owner_rank() == mesh.parallel_rank() ) {

    const stk::mesh::PartVector no_parts;

    stk::mesh::Entity & new_node =
      mesh.declare_entity(stk::mesh::Node, new_node_id, no_parts);

    mesh.destroy_relation(*right_element, *old_node);
    mesh.declare_relation(*right_element, new_node, right_ordinal);
  }

  //copy parts

  //copy field data

  mesh.modification_end();

  if ( right_element ) {
    stk::mesh::PairIterRelation rel = right_element->relations();
    stk::mesh::Entity & new_node = * (rel.first[right_ordinal].entity());

    STKUNIT_EXPECT_TRUE ( new_node.identifier() == new_node_id );
  }
}

//----------------------------------------------------------------------------
