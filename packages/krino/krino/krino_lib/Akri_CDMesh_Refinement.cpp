// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <Akri_AdaptivityHelpers.hpp>
#include <Akri_AuxMetaData.hpp>
#include <Akri_CDFEM_Snapper.hpp>
#include <Akri_CDMesh.hpp>
#include <Akri_CDMesh_Utils.hpp>
#include <Akri_DiagWriter.hpp>
#include <Akri_FieldRef.hpp>
#include <Akri_InterfaceGeometry.hpp>
#include <Akri_Intersection_Points.hpp>
#include <Akri_NodeToCapturedDomains.hpp>
#include <stk_mesh/base/FieldBLAS.hpp>
#include <stk_mesh/base/FieldParallel.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/Relation.hpp>

namespace krino {

namespace {
  void set_refine_if_not_reached_max_refine(stk::mesh::Entity elem,
    const int interface_max_refine_level,
    FieldRef elem_marker,
    FieldRef refine_level_field,
    FieldRef transition_element_field)
  {
    int & marker = *field_data<int>(elem_marker, elem);
    const int refine_level = *field_data<int>(refine_level_field, elem);
    const int transition_element = *field_data<int>(transition_element_field, elem);

    if (refine_level >= transition_element+interface_max_refine_level )
    {
      marker = Refinement_Marker::NOTHING;
    }
    else
    {
      marker = Refinement_Marker::REFINE;
    }
  }
}

static bool node_is_snapped_to_interface(stk::mesh::Entity node,
    const InterfaceID & interface,
    const std::unordered_map<stk::mesh::Entity, std::set<InterfaceID>> & nodesSnappedInterfaces)
{
  auto nodeSnappedInterfaces = nodesSnappedInterfaces.find(node);
  const bool nodeIsSnapped = nodeSnappedInterfaces != nodesSnappedInterfaces.end() &&
      nodeSnappedInterfaces->second.find(interface) != nodeSnappedInterfaces->second.end();
  return nodeIsSnapped;
}

void
refine_edges_with_multiple_unsnapped_crossings(const stk::mesh::BulkData& mesh,
    const std::vector<IntersectionPoint> & edgeIntersections,
    const int interface_max_refine_level,
    FieldRef elem_marker_field,
    FieldRef refine_level_field,
    FieldRef transition_element_field,
    const std::unordered_map<stk::mesh::Entity, std::set<InterfaceID>> & nodesSnappedInterfaces)
{
  // Refine any edge with multiple crossings that are not snapped to the nodes
  std::set<std::array<stk::mesh::Entity,2>> edgesWithUnsnappedCrossings;
  for (auto && edgeIntersection : edgeIntersections)
  {
    const EdgeIntersection edge(edgeIntersection);

    const InterfaceID & interface = edge.interface;
    const bool node1IsSnapped = node_is_snapped_to_interface(edge.nodes[0], interface, nodesSnappedInterfaces);
    const bool node2IsSnapped = node_is_snapped_to_interface(edge.nodes[1], interface, nodesSnappedInterfaces);
    if (!node1IsSnapped && !node2IsSnapped)
    {
      const auto insertion = edgesWithUnsnappedCrossings.insert(edge.nodes);
      const bool alreadyInSet = !insertion.second;
      if (alreadyInSet)
      {
        std::vector<stk::mesh::Entity> edge_elems;
        stk::mesh::get_entities_through_relations(mesh, {edge.nodes[0], edge.nodes[1]},
            stk::topology::ELEMENT_RANK, edge_elems);
        for (auto && elem : edge_elems)
        {
          set_refine_if_not_reached_max_refine(elem, interface_max_refine_level, elem_marker_field, refine_level_field, transition_element_field);
        }
      }
    }
  }
}

void
refine_edges_with_nodes_with_multiple_snapped_interfaces(const stk::mesh::BulkData& mesh,
    const std::vector<IntersectionPoint> & edgeIntersections,
    const int interface_max_refine_level,
    FieldRef elem_marker_field,
    FieldRef refine_level_field,
    FieldRef transition_element_field,
    const std::unordered_map<stk::mesh::Entity, std::set<InterfaceID>> & node_snapped_interfaces)
{
  for (auto && edgeIntersection : edgeIntersections)
  {
    const EdgeIntersection edge(edgeIntersection);

    auto node1_snapped_interfaces = node_snapped_interfaces.find(edge.nodes[0]);
    auto node2_snapped_interfaces = node_snapped_interfaces.find(edge.nodes[1]);
    const unsigned num_node1_interfaces = (node1_snapped_interfaces != node_snapped_interfaces.end()) ?
        node1_snapped_interfaces->second.size() : 0;
    const unsigned num_node2_interfaces = (node2_snapped_interfaces != node_snapped_interfaces.end()) ?
        node2_snapped_interfaces->second.size() : 0;
    if (num_node1_interfaces > 1 || num_node2_interfaces > 1)
    {
      std::vector<stk::mesh::Entity> edge_elems;
      stk::mesh::get_entities_through_relations(mesh, {edge.nodes[0], edge.nodes[1]},
          stk::topology::ELEMENT_RANK, edge_elems);
      for (auto && elem : edge_elems)
      {
        set_refine_if_not_reached_max_refine(elem, interface_max_refine_level, elem_marker_field, refine_level_field, transition_element_field);
      }
    }
  }
}

void
determine_which_interfaces_snap_to_each_node_and_unsnappable_nodes(const stk::mesh::BulkData& mesh,
    const std::vector<IntersectionPoint> & edgeIntersections,
    const std::vector<InterfaceID> & active_interface_ids,
    const CDFEM_Snapper & snapper,
    FieldRef node_marker_field,
    std::unordered_map<stk::mesh::Entity, std::set<InterfaceID>> & node_snapped_interfaces,
    std::set<stk::mesh::Entity> & unsnappable_nodes)
{
  for (auto && interface : active_interface_ids)
  {
    stk::mesh::field_fill(0, node_marker_field);
    for (auto && edgeIntersection : edgeIntersections)
    {
      const EdgeIntersection edge(edgeIntersection);

      int & node1_marker = *field_data<int>(node_marker_field, edge.nodes[0]);
      int & node2_marker = *field_data<int>(node_marker_field, edge.nodes[1]);
      if (snapper.snap_lo(edge.crossingLocation))
      {
        if (parts_are_compatible_for_snapping(mesh, edge.nodes[0], edge.nodes[1]))
          node1_marker = 2;
        else
          node1_marker = std::max(node1_marker, 1);
      }
      if (snapper.snap_hi(edge.crossingLocation))
      {
        if (parts_are_compatible_for_snapping(mesh, edge.nodes[1], edge.nodes[0]))
          node2_marker = 2;
        else
          node2_marker = std::max(node2_marker, 1);
      }
    }
    stk::mesh::parallel_max(mesh, {&node_marker_field.field()});

    for(const auto & b_ptr : mesh.buckets( stk::topology::NODE_RANK ))
    {
      stk::mesh::Bucket & b = *b_ptr;
      int * node_marker = field_data<int>(node_marker_field, b);
      for(unsigned i=0; i < b.size(); ++i)
      {
        if (node_marker[i] == 1)
        {
          unsnappable_nodes.insert(b[i]);
        }
        else if (node_marker[i] == 2)
        {
          node_snapped_interfaces[b[i]].insert(interface);
        }
      }
    }
  }
}

void
resolve_fine_features(const stk::mesh::BulkData& mesh,
    const std::vector<IntersectionPoint> & edgeIntersections,
    const std::vector<InterfaceID> & active_interface_ids,
    const CDFEM_Snapper & snapper,
    const int interface_max_refine_level,
    FieldRef elem_marker_field,
    FieldRef node_marker_field,
    FieldRef refine_level_field,
    FieldRef transition_element_field)
{
  std::unordered_map<stk::mesh::Entity, std::set<InterfaceID>> node_snapped_interfaces;
  std::set<stk::mesh::Entity> unsnappable_nodes;

  determine_which_interfaces_snap_to_each_node_and_unsnappable_nodes(mesh, edgeIntersections, active_interface_ids, snapper, node_marker_field, node_snapped_interfaces, unsnappable_nodes);

  // Attempt at minimally aggressive. Works pretty well and has has lower element counts.
  refine_edges_with_multiple_unsnapped_crossings(mesh, edgeIntersections, interface_max_refine_level, elem_marker_field, refine_level_field, transition_element_field, node_snapped_interfaces);
  //refine_edges_with_unsnappable_nodes(interface_max_refine_level, elem_marker_field, refine_level_field, transition_element_field, unsnappable_nodes);
  refine_edges_with_nodes_with_multiple_snapped_interfaces(mesh, edgeIntersections, interface_max_refine_level, elem_marker_field, refine_level_field, transition_element_field, node_snapped_interfaces);
}

void
mark_nearest_node_on_cut_edges(const stk::mesh::BulkData& mesh,
    const std::vector<IntersectionPoint> & edgeIntersections,
    FieldRef node_marker_field)
{
  const double overlap = 0.25;

  stk::mesh::field_fill(0, node_marker_field);

  for (auto && edgeIntersection : edgeIntersections)
  {
    const EdgeIntersection edge(edgeIntersection);

    if (edge.crossingLocation < 0.5+overlap)
      *field_data<int>(node_marker_field, edge.nodes[0]) = 1;
    if (edge.crossingLocation > 0.5-overlap)
      *field_data<int>(node_marker_field, edge.nodes[1]) = 1;
  }
  stk::mesh::parallel_max(mesh, {&node_marker_field.field()});
}

int determine_refinement_marker(const bool haveCrossing, const int numRefinements, const int interfaceMinRefineLevel, const int elementRefineLevel, const int transitionElement)
{
  int marker = Refinement_Marker::NOTHING;
  const int targetRefineLevel = haveCrossing ? interfaceMinRefineLevel : 0;
  if (elementRefineLevel < targetRefineLevel+transitionElement)
  {
    marker = Refinement_Marker::REFINE;
  }
  else if (numRefinements < interfaceMinRefineLevel &&
      elementRefineLevel > targetRefineLevel+transitionElement)
  {
    // Stop coarsening after num_levels of refinement to avoid infinite looping
    // from the interface position moving between elements because of snapping changes
    // with refinement
    marker = Refinement_Marker::COARSEN;
  }
  return marker;
}

void
mark_possible_cut_elements_for_adaptivity(const stk::mesh::BulkData& mesh,
      const InterfaceGeometry & interfaceGeometry,
      const AuxMetaData& aux_meta,
      const CDFEM_Support & cdfem_support,
      const std::string & marker_field_name,
      const int numRefinements)
{
/* %TRACE[SPEC]% */ Tracespec trace__("CDMesh::mark_interface_elements_for_adaptivity(const std::string & marker_field_name, const int num_refinements)"); /* %TRACE% */

  // This refinement strategy cuts elements by the user-specified number of adapt levels
  // before the conformal decomposition.

  const FieldRef elementMarkerField = aux_meta.get_field(stk::topology::ELEMENT_RANK, marker_field_name, stk::mesh::StateNew);
  const FieldRef elementRefineLevelField = aux_meta.get_field(stk::topology::ELEMENT_RANK, "refine_level");
  const std::string transitionElementFieldName = (mesh.mesh_meta_data().spatial_dimension() == 2) ?
      "transition_element" : "transition_element_3";
  const FieldRef transitionElementField = aux_meta.get_field(stk::topology::ELEMENT_RANK, transitionElementFieldName);

  const int interfaceMinRefineLevel = cdfem_support.get_interface_minimum_refinement_level();

  std::vector<stk::mesh::Entity> possibleCutElems = interfaceGeometry.get_possibly_cut_elements(mesh);
  for( auto&& elem : possibleCutElems )
  {
    bool possibleCrossing = true;
    const int elementRefineLevel = *field_data<int>(elementRefineLevelField, elem);
    const int transitionElement = *field_data<int>(transitionElementField, elem);

    int & marker = *field_data<int>(elementMarkerField, elem);
    marker = determine_refinement_marker(possibleCrossing, numRefinements, interfaceMinRefineLevel, elementRefineLevel, transitionElement);
  }
}

void write_refinement_level_sizes(const stk::mesh::BulkData& mesh,
    const AuxMetaData& aux_meta,
    const FieldRef coordsField,
    const FieldRef elementRefineLevelField,
    const FieldRef transitionElementField,
    const std::vector<stk::mesh::Entity> & elements,
    const int interfaceMaxRefineLevel)
{
  std::vector<double> min_edge_lengths(interfaceMaxRefineLevel+1, std::numeric_limits<double>::max());
  std::vector<double> max_edge_lengths(interfaceMaxRefineLevel+1, 0.0);

  for( auto&& elem : elements )
  {
    const int transitionElement = *field_data<int>(transitionElementField, elem);

    if (!transitionElement)
    {
      const int elementRefineLevel = *field_data<int>(elementRefineLevelField, elem);
      const stk::topology stk_topology = mesh.bucket(elem).topology();
      const stk::mesh::Entity * const elem_nodes = mesh.begin(elem, stk::topology::NODE_RANK);
      std::vector<stk::mesh::Entity> edge_nodes;
      for (unsigned i = 0; i < stk_topology.num_edges(); ++i)
      {
        edge_nodes.resize(stk_topology.edge_topology(i).num_nodes());
        stk_topology.edge_nodes(elem_nodes, i, edge_nodes.data());

        const Vector3d edge_node1_coords(field_data<double>(coordsField, edge_nodes[0]), mesh.mesh_meta_data().spatial_dimension());
        const Vector3d edge_node2_coords(field_data<double>(coordsField, edge_nodes[1]), mesh.mesh_meta_data().spatial_dimension());
        const double length = (edge_node2_coords-edge_node1_coords).length();

        min_edge_lengths[elementRefineLevel] = std::min(min_edge_lengths[elementRefineLevel], length);
        max_edge_lengths[elementRefineLevel] = std::max(max_edge_lengths[elementRefineLevel], length);
      }
    }
  }

  for (int i=0; i<interfaceMaxRefineLevel; ++i)
  {
    krinolog << "Min and Max sizes for refinement level " << i <<  " " << min_edge_lengths[i] << " " << max_edge_lengths[i] << stk::diag::dendl;
  }
}

bool element_has_marked_node(const stk::mesh::BulkData& mesh, stk::mesh::Entity elem, const FieldRef nodeMarkerField)
{
  const unsigned num_nodes = mesh.num_nodes(elem);
  const stk::mesh::Entity * const elemNodes = mesh.begin_nodes(elem);
  for(unsigned i=0; i < num_nodes; ++i)
  {
    const int nodeMarker = *field_data<int>(nodeMarkerField, elemNodes[i]);
    if(nodeMarker)
      return true;
  }
  return false;
}

void
mark_interface_elements_for_adaptivity(const stk::mesh::BulkData& mesh,
      const InterfaceGeometry & interfaceGeometry,
      const std::vector<InterfaceID> & active_interface_ids,
      const CDFEM_Snapper & snapper,
      const AuxMetaData& aux_meta,
      const CDFEM_Support & cdfem_support,
      const FieldRef coordsField,
      const std::string & marker_field_name,
      const int numRefinements)
{
/* %TRACE[SPEC]% */ Tracespec trace__("CDMesh::mark_interface_elements_for_adaptivity(const std::string & marker_field_name, const int num_refinements)"); /* %TRACE% */

  // This refinement strategy cuts elements by the user-specified number of adapt levels
  // before the conformal decomposition.

  const FieldRef elementMarkerField = aux_meta.get_field(stk::topology::ELEMENT_RANK, marker_field_name, stk::mesh::StateNew);
  const FieldRef elementRefineLevelField = aux_meta.get_field(stk::topology::ELEMENT_RANK, "refine_level");
  const std::string transitionElementFieldName = (mesh.mesh_meta_data().spatial_dimension() == 2) ?
      "transition_element" : "transition_element_3";
  const FieldRef transitionElementField = aux_meta.get_field(stk::topology::ELEMENT_RANK, transitionElementFieldName);

  const stk::mesh::Selector active_selector(cdfem_support.get_active_part());
  const stk::mesh::Selector locally_owned_selector(cdfem_support.get_locally_owned_part());
  const stk::mesh::Selector parent_or_child_selector = cdfem_support.get_child_part() | cdfem_support.get_parent_part();
  const int interfaceMinRefineLevel = cdfem_support.get_interface_minimum_refinement_level();
  const int interfaceMaxRefineLevel = cdfem_support.get_interface_maximum_refinement_level();
  std::vector<stk::mesh::Entity> entities;
  std::vector<stk::mesh::Entity> children;

  const NodeToCapturedDomainsMap nodesToCapturedDomains;
  const std::vector<IntersectionPoint> edgeIntersections = interfaceGeometry.get_edge_intersection_points(mesh, nodesToCapturedDomains);

  FieldRef nodeMarkerField = aux_meta.get_field(stk::topology::NODE_RANK, marker_field_name, stk::mesh::StateNew);
  mark_nearest_node_on_cut_edges(mesh, edgeIntersections, nodeMarkerField);

  std::vector<double> min_edge_lengths(interfaceMaxRefineLevel+1, std::numeric_limits<double>::max());
  std::vector<double> max_edge_lengths(interfaceMaxRefineLevel+1, 0.0);

  stk::mesh::get_selected_entities( locally_owned_selector, mesh.buckets( stk::topology::ELEMENT_RANK ), entities );
  for( auto&& elem : entities )
  {
    bool hasCrossing = element_has_marked_node(mesh, elem, nodeMarkerField);

    const int elementRefineLevel = *field_data<int>(elementRefineLevelField, elem);
    const int transitionElement = *field_data<int>(transitionElementField, elem);

    int & marker = *field_data<int>(elementMarkerField, elem);
    marker = determine_refinement_marker(hasCrossing, numRefinements, interfaceMinRefineLevel, elementRefineLevel, transitionElement);
  }

  write_refinement_level_sizes(mesh, aux_meta, coordsField, elementRefineLevelField, transitionElementField, entities, interfaceMaxRefineLevel);

  if (interfaceMinRefineLevel > interfaceMaxRefineLevel)
  {
    resolve_fine_features(mesh, edgeIntersections, active_interface_ids, snapper, interfaceMaxRefineLevel, elementMarkerField, nodeMarkerField, elementRefineLevelField, transitionElementField);
  }
}

void
refine_edges_with_unsnappable_nodes(const stk::mesh::BulkData& mesh,
    const std::vector<IntersectionPoint> & edgeIntersections,
    const CDFEM_Snapper & snapper,
    const int interface_max_refine_level,
    FieldRef elem_marker_field,
    FieldRef refine_level_field,
    FieldRef transition_element_field,
    std::set<stk::mesh::Entity> unsnappable_nodes)
{
  for (auto && edgeIntersection : edgeIntersections)
  {
    const EdgeIntersection edge(edgeIntersection);

    if ((snapper.snap_lo(edge.crossingLocation) && unsnappable_nodes.find(edge.nodes[0]) != unsnappable_nodes.end()) ||
        (snapper.snap_hi(edge.crossingLocation) && unsnappable_nodes.find(edge.nodes[1]) != unsnappable_nodes.end()))
    {
      krinolog << "Refining unsnappable edge  " << mesh.identifier(edge.nodes[0]) << " " << mesh.identifier(edge.nodes[1]) << " " << debug_output(mesh, edgeIntersection) << stk::diag::dendl;
      std::vector<stk::mesh::Entity> edge_elems;
      stk::mesh::get_entities_through_relations(mesh, {edge.nodes[0], edge.nodes[1]},
          stk::topology::ELEMENT_RANK, edge_elems);
      for (auto && elem : edge_elems)
      {
        set_refine_if_not_reached_max_refine(elem, interface_max_refine_level, elem_marker_field, refine_level_field, transition_element_field);
      }
    }
  }
}

}
