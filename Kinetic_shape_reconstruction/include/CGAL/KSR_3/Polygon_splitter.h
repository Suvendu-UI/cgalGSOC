// Copyright (c) 2019 GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
// Author(s)     : Simon Giraudot

#ifndef CGAL_KSR_3_POLYGON_SPLITTER_H
#define CGAL_KSR_3_POLYGON_SPLITTER_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

// CGAL includes.
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_plus_2.h>
#include <CGAL/Constrained_triangulation_face_base_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>

// Internal includes.
#include <CGAL/KSR/utils.h>

namespace CGAL {
namespace KSR_3 {

template<typename Data_structure, typename GeomTraits>
class Polygon_splitter {

public:
  using Kernel = GeomTraits;

private:
  using FT         = typename Kernel::FT;
  using Point_2    = typename Kernel::Point_2;
  using Point_3    = typename Kernel::Point_3;
  using Line_2     = typename Kernel::Line_2;
  using Vector_2   = typename Kernel::Vector_2;
  using Triangle_2 = typename Kernel::Triangle_2;

  using PVertex = typename Data_structure::PVertex;
  using PFace   = typename Data_structure::PFace;

  using IVertex = typename Data_structure::IVertex;
  using IEdge   = typename Data_structure::IEdge;

  struct Vertex_info {
    PVertex pvertex;
    IVertex ivertex;
    Vertex_info() :
    pvertex(Data_structure::null_pvertex()),
    ivertex(Data_structure::null_ivertex())
    { }
  };

  struct Face_info {
    KSR::size_t index;
    Face_info() :
    index(KSR::uninitialized())
    { }
  };

  using VBI = CGAL::Triangulation_vertex_base_with_info_2<Vertex_info, Kernel>;
  using FBI = CGAL::Triangulation_face_base_with_info_2<Face_info, Kernel>;
  using CFB = CGAL::Constrained_triangulation_face_base_2<Kernel, FBI>;
  using TDS = CGAL::Triangulation_data_structure_2<VBI, CFB>;
  using TAG = CGAL::Exact_predicates_tag;
  using CDT = CGAL::Constrained_Delaunay_triangulation_2<Kernel, TDS, TAG>;
  using TRI = CGAL::Constrained_triangulation_plus_2<CDT>;
  using CID = typename TRI::Constraint_id;

  using Vertex_handle = typename TRI::Vertex_handle;
  using Face_handle   = typename TRI::Face_handle;
  using Edge          = typename TRI::Edge;

  using Mesh_3       = CGAL::Surface_mesh<Point_3>;
  using Vertex_index = typename Mesh_3::Vertex_index;
  using Face_index   = typename Mesh_3::Face_index;
  using Uchar_map    = typename Mesh_3::template Property_map<Face_index, unsigned char>;

  Data_structure& m_data;
  TRI m_cdt;
  std::set<PVertex> m_input;
  std::map<CID, IEdge> m_map_intersections;

public:
  Polygon_splitter(Data_structure& data) :
  m_data(data)
  { }

  void split_support_plane(const KSR::size_t support_plane_idx) {

    // Create cdt.
    std::vector<KSR::size_t> original_input;
    std::vector< std::vector<Point_2> > original_faces;
    initialize_cdt(support_plane_idx, original_input, original_faces);
    tag_cdt_exterior_faces();
    tag_cdt_interior_faces();
    dump(false, support_plane_idx);

    // Split polygons using cdt.
    m_data.clear_polygon_faces(support_plane_idx);
    initialize_new_pfaces(support_plane_idx, original_input, original_faces);

    // Set intersection adjacencies.
    reconnect_pvertices_to_ivertices();
    reconnect_pedges_to_iedges();
    set_new_adjacencies(support_plane_idx);
  }

private:

  void initialize_cdt(
    const KSR::size_t support_plane_idx,
    std::vector<KSR::size_t>& original_input,
    std::vector< std::vector<Point_2> >& original_faces) {

    // Insert pvertices.
    std::map<PVertex, Vertex_handle> vhs_map;
    const auto all_pvertices = m_data.pvertices(support_plane_idx);
    for (const auto pvertex : all_pvertices) {
      const auto vh = m_cdt.insert(m_data.point_2(pvertex));
      vh->info().pvertex = pvertex;
      m_input.insert(pvertex);
      vhs_map[pvertex] = vh;
    }

    // Insert pfaces and the corresponding constraints.
    original_faces.clear();
    original_input.clear();

    // std::vector<Vertex_handle> vhs;
    std::vector<Point_2> original_face;
    const auto all_pfaces = m_data.pfaces(support_plane_idx);
    for (const auto pface : all_pfaces) {
      const auto pvertices = m_data.pvertices_of_pface(pface);

      // vhs.clear();
      original_face.clear();
      for (const auto pvertex : pvertices) {
        CGAL_assertion(vhs_map.find(pvertex) != vhs_map.end());
        const auto vh = vhs_map.at(pvertex);
        original_face.push_back(vh->point());
        // vhs.push_back(vh);
      }

      original_faces.push_back(original_face);
      original_input.push_back(m_data.input(pface));

      // TODO: Why we cannot use vhs directly here? That should be more precise!
      // vhs.push_back(vhs.front());
      // const auto cid = m_cdt.insert_constraint(vhs.begin(), vhs.end());
      original_face.push_back(original_face.front());
      const auto cid = m_cdt.insert_constraint(original_face.begin(), original_face.end());
      m_map_intersections.insert(std::make_pair(cid, Data_structure::null_iedge()));
    }

    // Then, add intersection vertices + constraints.
    const auto& iedges = m_data.iedges(support_plane_idx);
    for (const auto& iedge : iedges) {
      const auto source = m_data.source(iedge);
      const auto target = m_data.target(iedge);

      const auto vsource = m_cdt.insert(m_data.to_2d(support_plane_idx, source));
      vsource->info().ivertex = source;
      const auto vtarget = m_cdt.insert(m_data.to_2d(support_plane_idx, target));
      vtarget->info().ivertex = target;

      const auto cid = m_cdt.insert_constraint(vsource, vtarget);
      m_map_intersections.insert(std::make_pair(cid, iedge));
    }
  }

  // All exterior faces are tagged by KSR::no_element().
  void tag_cdt_exterior_faces() {

    std::queue<Face_handle> todo;
    todo.push(m_cdt.incident_faces(m_cdt.infinite_vertex()));
    while (!todo.empty()) {
      const auto fh = todo.front();
      todo.pop();
      if (fh->info().index != KSR::uninitialized()) {
        continue;
      }
      fh->info().index = KSR::no_element();

      for (std::size_t i = 0; i < 3; ++i) {
        const auto next = fh->neighbor(i);
        const auto edge = std::make_pair(fh, i);
        const bool is_border_edge = is_border(edge);
        if (!is_border_edge) {
          todo.push(next);
        }
      }
    }
    CGAL_assertion(todo.size() == 0);
  }

  const bool is_border(const Edge& edge) const {

    if (!m_cdt.is_constrained(edge)) {
      return false;
    }

    const std::size_t im = (edge.second + 2) % 3;
    const std::size_t ip = (edge.second + 1) % 3;

    const auto vm = edge.first->vertex(im);
    const auto vp = edge.first->vertex(ip);

    const auto ctx_begin = m_cdt.contexts_begin(vp, vm);
    const auto ctx_end = m_cdt.contexts_end(vp, vm);

    for (auto cit = ctx_begin; cit != ctx_end; ++cit) {
      const auto iter = m_map_intersections.find(cit->id());
      if (iter == m_map_intersections.end()) {
        continue;
      }
      if (iter->second == Data_structure::null_iedge()) {
        return true;
      }
    }
    return false;
  }

  // All enterior faces are tagged by face_index.
  void tag_cdt_interior_faces() {

    KSR::size_t face_index = 0;
    std::queue<Face_handle> todo;
    for (auto fit = m_cdt.finite_faces_begin(); fit != m_cdt.finite_faces_end(); ++fit) {
      CGAL_assertion(todo.size() == 0);
      if (fit->info().index != KSR::uninitialized()) {
        continue;
      }

      todo.push(fit);
      KSR::size_t num_faces = 0;
      while (!todo.empty()) {
        const auto fh = todo.front();
        todo.pop();
        if (fh->info().index != KSR::uninitialized()) {
          continue;
        }
        fh->info().index = face_index;
        ++num_faces;

        for (std::size_t i = 0; i < 3; ++i) {
          const auto next = fh->neighbor(i);
          const auto edge = std::make_pair(fh, i);
          const bool is_constrained_edge = m_cdt.is_constrained(edge);
          if (!is_constrained_edge) {
            todo.push(next);
          }
        }
      }
      ++face_index;
      CGAL_assertion(todo.size() == 0);
    }
  }

  void initialize_new_pfaces(
    const KSR::size_t support_plane_idx,
    const std::vector<KSR::size_t>& original_input,
    const std::vector< std::vector<Point_2> >& original_faces) {

    std::set<KSR::size_t> done;
    for (auto fit = m_cdt.finite_faces_begin(); fit != m_cdt.finite_faces_end(); ++fit) {
      CGAL_assertion(fit->info().index != KSR::uninitialized());
      if (fit->info().index == KSR::no_element()) { // skip exterior faces
        continue;
      }

      // Search for a constrained edge.
      Edge edge;
      for (std::size_t i = 0; i < 3; ++i) {
        edge = std::make_pair(fit, i);
        if (m_cdt.is_constrained(edge)) {
          break;
        }
      }

      // Skip pure interior faces.
      if (!m_cdt.is_constrained(edge)) {
        continue;
      }

      // If face index is already a part of the set, skip.
      const auto fh = edge.first;
      if (!done.insert(fh->info().index).second) {
        continue;
      }

      // Start from the constrained edge and traverse all constrained edges / boundary
      // of the triangulation part that is tagged with the same face index.
      // While traversing, add all missing pvertices.
      auto curr = edge;
      std::vector<PVertex> new_pvertices;
      do {
        const auto curr_face = curr.first;
        const int idx = curr.second;

        const auto source = curr_face->vertex(m_cdt.ccw(idx));
        const auto target = curr_face->vertex(m_cdt.cw (idx));
        if (source->info().pvertex == Data_structure::null_pvertex()) {
          source->info().pvertex =
            m_data.add_pvertex(support_plane_idx, source->point());
        }
        new_pvertices.push_back(source->info().pvertex);

        // Search for the next constrained edge.
        auto next = std::make_pair(curr_face, m_cdt.ccw(idx));
        while (!m_cdt.is_constrained(next)) {

          const auto next_face = next.first->neighbor(next.second);
          // Should be the same original polygon.
          CGAL_assertion(next_face->info().index == edge.first->info().index);

          const int next_idx = m_cdt.ccw(next_face->index(next.first));
          next = std::make_pair(next_face, next_idx);
        }
        // Check wether next source == previous target.
        CGAL_assertion(next.first->vertex(m_cdt.ccw(next.second)) == target);
        curr = next;

      } while (curr != edge);
      CGAL_assertion(curr == edge);

      // Add a new pface.
      const auto pface = m_data.add_pface(new_pvertices);
      CGAL_assertion(pface != PFace());

      std::size_t original_idx = 0;
      if (original_faces.size() != 1) {
        // TODO: locate centroid of the face among the different
        // original faces to recover the input index.
        CGAL_assertion_msg(false,
        "TODO: POLYGON SPLITTER, FIX CASE WITH MULTIPLE ORIGINAL FACES!");
      }
      m_data.input(pface) = original_input[original_idx];
    }
  }

  void reconnect_pvertices_to_ivertices() {

    // Reconnect only those, which have already been connected.
    for (auto vit = m_cdt.finite_vertices_begin(); vit != m_cdt.finite_vertices_end(); ++vit) {
      if (vit->info().pvertex != Data_structure::null_pvertex() &&
          vit->info().ivertex != Data_structure::null_ivertex()) {
        m_data.connect(vit->info().pvertex, vit->info().ivertex);
      }
    }
  }

  void reconnect_pedges_to_iedges() {

    // Reconnect only those, which have already been connected.
    for (const auto& item : m_map_intersections) {
      const auto& cid   = item.first;
      const auto& iedge = item.second;

      if (iedge == Data_structure::null_iedge()) {
        continue;
      }
      CGAL_assertion(iedge != Data_structure::null_iedge());

      auto vit = m_cdt.vertices_in_constraint_begin(cid);
      while (true) {
        auto next = vit; ++next;
        if (next == m_cdt.vertices_in_constraint_end(cid)) { break; }
        const auto a = *vit;
        const auto b = *next;
        vit = next;

        if (
          a->info().pvertex == Data_structure::null_pvertex() ||
          b->info().pvertex == Data_structure::null_pvertex()) {
          continue;
        }
        CGAL_assertion(a->info().pvertex != Data_structure::null_pvertex());
        CGAL_assertion(b->info().pvertex != Data_structure::null_pvertex());
        m_data.connect(a->info().pvertex, b->info().pvertex, iedge);
      }
    }
  }

  void set_new_adjacencies(
    const KSR::size_t support_plane_idx) {

    const auto all_pvertices = m_data.pvertices(support_plane_idx);
    for (const auto pvertex : all_pvertices) {

      bool is_frozen = false;
      auto iedge = Data_structure::null_iedge();
      std::pair<PVertex, PVertex> neighbors(
        Data_structure::null_pvertex(), Data_structure::null_pvertex());

      // Search for a frozen pvertex.
      const auto pedges = m_data.pedges_around_pvertex(pvertex);
      for (const auto pedge : pedges) {

        if (m_data.has_iedge(pedge)) {
          if (iedge == Data_structure::null_iedge()) {
            iedge = m_data.iedge(pedge);
          } else {
            is_frozen = true;
            break;
          }
        } else {
          const auto opposite = m_data.opposite(pedge, pvertex);
          if (neighbors.first == Data_structure::null_pvertex()) {
            neighbors.first = opposite;
          } else {
            CGAL_assertion(neighbors.second == Data_structure::null_pvertex());
            neighbors.second = opposite;
          }
        }
      }

      // Several incident intersections = frozen pvertex.
      if (is_frozen) {
        m_data.direction(pvertex) = CGAL::NULL_VECTOR;
        continue;
      }

      // No incident intersections = keep initial direction.
      if (iedge == Data_structure::null_iedge()) {
        continue;
      }
      m_data.connect(pvertex, iedge);
      CGAL_assertion(
        neighbors.first  != Data_structure::null_pvertex() &&
        neighbors.second != Data_structure::null_pvertex());

      // Set future direction.
      bool first_okay = (m_input.find(neighbors.first) != m_input.end());
      auto last = pvertex;
      auto curr = neighbors.first;
      while (!first_okay) {
        PVertex next, ignored;
        std::tie(next, ignored) = m_data.border_prev_and_next(curr);
        if (next == last) {
          std::swap(next, ignored);
        }
        CGAL_assertion(ignored == last);

        last = curr; curr = next;
        if (m_input.find(curr) != m_input.end()) {
          neighbors.first = curr;
          first_okay = true;
        }
      }

      bool second_okay = (m_input.find(neighbors.second) != m_input.end());
      last = pvertex;
      curr = neighbors.second;
      while (!second_okay) {
        PVertex next, ignored;
        std::tie(next, ignored) = m_data.border_prev_and_next(curr);
        if (next == last) {
          std::swap(next, ignored);
        }
        CGAL_assertion(ignored == last);

        last = curr; curr = next;
        if (m_input.find(curr) != m_input.end()) {
          neighbors.second = curr;
          second_okay = true;
        }
      }

      const Line_2 future_line(
        m_data.point_2(neighbors.first, FT(1)), m_data.point_2(neighbors.second, FT(1)));
      const auto intersection_line = m_data.segment_2(support_plane_idx, iedge).supporting_line();
      const Point_2 inter = KSR::intersection<Point_2>(intersection_line, future_line);
      m_data.direction(pvertex) = Vector_2(m_data.point_2(pvertex, FT(0)), inter);
    }
  }

  void dump(
    const bool dump_data,
    const KSR::size_t support_plane_idx) {
    if (!dump_data) return;

    Mesh_3 mesh;
    Uchar_map red   = mesh.template add_property_map<Face_index, unsigned char>("red", 0).first;
    Uchar_map green = mesh.template add_property_map<Face_index, unsigned char>("green", 0).first;
    Uchar_map blue  = mesh.template add_property_map<Face_index, unsigned char>("blue", 0).first;

    std::map<Vertex_handle, Vertex_index> map_v2i;
    for (auto vit = m_cdt.finite_vertices_begin(); vit != m_cdt.finite_vertices_end(); ++vit) {
      map_v2i.insert(std::make_pair(
        vit, mesh.add_vertex(m_data.support_plane(support_plane_idx).to_3d(vit->point()))));
    }

    for (auto fit = m_cdt.finite_faces_begin(); fit != m_cdt.finite_faces_end(); ++fit) {
      std::array<Vertex_index, 3> vertices;
      for (std::size_t i = 0; i < 3; ++i) {
        vertices[i] = map_v2i[fit->vertex(i)];
      }

      const auto face = mesh.add_face(vertices);
      CGAL::Random rand(fit->info().index);
      if (fit->info().index != KSR::no_element()) {
        red[face]   = (unsigned char)(rand.get_int(32, 192));
        green[face] = (unsigned char)(rand.get_int(32, 192));
        blue[face]  = (unsigned char)(rand.get_int(32, 192));
      }
    }

    const std::string filename = "face_" + std::to_string(support_plane_idx) + ".ply";
    std::ofstream out(filename);
    out.precision(20);
    CGAL::write_ply(out, mesh);
    out.close();
  }
};

} // namespace KSR_3
} // namespace CGAL

#endif // CGAL_KSR_3_POLYGON_SPLITTER_H
