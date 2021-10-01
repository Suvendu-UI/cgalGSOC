// Copyright (c) 2019 GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Simon Giraudot, Dmitry Anisimov

#ifndef CGAL_KSR_DEBUG_H
#define CGAL_KSR_DEBUG_H

// #include <CGAL/license/Kinetic_shape_reconstruction.h>

#if defined(WIN32) || defined(_WIN32)
#define _NL_ "\r\n"
#else
#define _NL_ "\n"
#endif

// STL includes.
#include <fstream>

// CGAL includes.
#include <CGAL/Point_set_3.h>
#include <CGAL/Point_set_3/IO.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Surface_mesh/IO.h>
#include <CGAL/Random.h>

// Internal includes.
#include <CGAL/KSR/utils.h>

namespace CGAL {
namespace KSR_3 {

const std::tuple<unsigned char, unsigned char, unsigned char>
get_idx_color(const std::size_t idx) {

  CGAL::Random rand(idx);
  return std::make_tuple(
    static_cast<unsigned char>(rand.get_int(32, 192)),
    static_cast<unsigned char>(rand.get_int(32, 192)),
    static_cast<unsigned char>(rand.get_int(32, 192)));
}

template<typename DS>
void dump_intersection_edges(const DS& data, const std::string tag = std::string()) {

  const std::string filename = (tag != std::string() ? tag + "-" : "") + "intersection-edges.polylines.txt";
  std::ofstream out(filename);
  out.precision(20);

  for (const auto iedge : data.iedges()) {
    out << "2 " << data.segment_3(iedge) << std::endl;
  }
  out.close();
}

template<typename DS>
void dump_segmented_edges(const DS& data, const std::string tag = std::string()) {

  std::vector<std::ofstream*> out;
  for (std::size_t i = 0; i < data.nb_intersection_lines(); ++i) {
    const std::string filename = (tag != std::string() ? tag + "-" : "") + "intersection-line-" + std::to_string(i) + ".polylines.txt";
    out.push_back(new std::ofstream(filename));
    out.back()->precision(20);
  }

  for (const auto iedge : data.iedges()) {
    CGAL_assertion(data.line_idx(iedge) != KSR::no_element());
    *(out[data.line_idx(iedge)]) << "2 " << data.segment_3(iedge) << std::endl;
  }

  for (std::ofstream* o : out) {
    delete o;
  }
}

template<typename DS>
void dump_constrained_edges(const DS& data, const std::string tag = std::string()) {

  const std::string filename = (tag != std::string() ? tag + "-" : "") + "constrained-edges.polylines.txt";
  std::ofstream out(filename);
  out.precision(20);

  for (std::size_t i = 0; i < data.number_of_support_planes(); ++i) {
    for (const auto pedge : data.pedges(i)) {
      if (data.has_iedge(pedge)) {
        out << "2 " << data.segment_3(pedge) << std::endl;
      }
    }
  }
  out.close();
}

template<typename DS>
void dump_2d_surface_mesh(
  const DS& data,
  const std::size_t support_plane_idx,
  const std::string tag = std::string()) {

  using Point_3      = typename DS::Kernel::Point_3;
  using Mesh         = CGAL::Surface_mesh<Point_3>;
  using Face_index   = typename Mesh::Face_index;
  using Vertex_index = typename Mesh::Vertex_index;
  using Uchar_map    = typename Mesh::template Property_map<Face_index, unsigned char>;

  Mesh mesh;
  Uchar_map red   = mesh.template add_property_map<Face_index, unsigned char>("red", 0).first;
  Uchar_map green = mesh.template add_property_map<Face_index, unsigned char>("green", 0).first;
  Uchar_map blue  = mesh.template add_property_map<Face_index, unsigned char>("blue", 0).first;

  std::vector<Vertex_index> vertices;
  std::vector<Vertex_index> map_vertices;

  map_vertices.clear();
  for (const auto pvertex : data.pvertices(support_plane_idx)) {
    if (map_vertices.size() <= pvertex.second) {
      map_vertices.resize(pvertex.second + 1);
    }
    map_vertices[pvertex.second] = mesh.add_vertex(data.point_3(pvertex));
  }

  for (const auto pface : data.pfaces(support_plane_idx)) {
    vertices.clear();
    for (const auto pvertex : data.pvertices_of_pface(pface)) {
      vertices.push_back(map_vertices[pvertex.second]);
    }

    CGAL_assertion(vertices.size() >= 3);
    const auto face = mesh.add_face(vertices);
    CGAL_assertion(face != Mesh::null_face());
    std::tie(red[face], green[face], blue[face]) =
      get_idx_color(support_plane_idx * (pface.second + 1));
  }

  const std::string filename = (tag != std::string() ? tag + "-" : "") + "polygons.ply";
  std::ofstream out(filename);
  out.precision(20);
  CGAL::IO::write_PLY(out, mesh);
  out.close();
}

template<typename DS>
void dump_polygons(const DS& data, const std::string tag = std::string()) {

  using Point_3      = typename DS::Kernel::Point_3;
  using Mesh         = CGAL::Surface_mesh<Point_3>;
  using Face_index   = typename Mesh::Face_index;
  using Vertex_index = typename Mesh::Vertex_index;
  using Uchar_map    = typename Mesh::template Property_map<Face_index, unsigned char>;

  Mesh mesh;
  Uchar_map red   = mesh.template add_property_map<Face_index, unsigned char>("red", 0).first;
  Uchar_map green = mesh.template add_property_map<Face_index, unsigned char>("green", 0).first;
  Uchar_map blue  = mesh.template add_property_map<Face_index, unsigned char>("blue", 0).first;

  Mesh bbox_mesh;
  Uchar_map bbox_red   = bbox_mesh.template add_property_map<Face_index, unsigned char>("red", 0).first;
  Uchar_map bbox_green = bbox_mesh.template add_property_map<Face_index, unsigned char>("green", 0).first;
  Uchar_map bbox_blue  = bbox_mesh.template add_property_map<Face_index, unsigned char>("blue", 0).first;

  std::vector<Vertex_index> vertices;
  std::vector<Vertex_index> map_vertices;

  for (std::size_t i = 0; i < data.number_of_support_planes(); ++i) {
    if (data.is_bbox_support_plane(i)) {

      map_vertices.clear();
      for (const auto pvertex : data.pvertices(i)) {
        if (map_vertices.size() <= pvertex.second) {
          map_vertices.resize(pvertex.second + 1);
        }
        map_vertices[pvertex.second] = bbox_mesh.add_vertex(data.point_3(pvertex));
      }

      for (const auto pface : data.pfaces(i)) {
        vertices.clear();
        for (const auto pvertex : data.pvertices_of_pface(pface)) {
          vertices.push_back(map_vertices[pvertex.second]);
        }

        CGAL_assertion(vertices.size() >= 3);
        const auto face = bbox_mesh.add_face(vertices);
        CGAL_assertion(face != Mesh::null_face());
        std::tie(bbox_red[face], bbox_green[face], bbox_blue[face]) =
          get_idx_color((i + 1) * (pface.second + 1));
      }

    } else {

      map_vertices.clear();
      for (const auto pvertex : data.pvertices(i)) {
        if (map_vertices.size() <= pvertex.second) {
          map_vertices.resize(pvertex.second + 1);
        }
        map_vertices[pvertex.second] = mesh.add_vertex(data.point_3(pvertex));
      }

      for (const auto pface : data.pfaces(i)) {
        vertices.clear();
        for (const auto pvertex : data.pvertices_of_pface(pface)) {
          vertices.push_back(map_vertices[pvertex.second]);
        }

        CGAL_assertion(vertices.size() >= 3);
        const auto face = mesh.add_face(vertices);
        CGAL_assertion(face != Mesh::null_face());
        std::tie(red[face], green[face], blue[face]) =
          get_idx_color(i * (pface.second + 1));
      }
    }
  }

  const std::string filename = (tag != std::string() ? tag + "-" : "") + "polygons.ply";
  std::ofstream out(filename);
  out.precision(20);
  CGAL::IO::write_PLY(out, mesh);
  out.close();

#if false

  const std::string bbox_filename = (tag != std::string() ? tag + "-" : "") + "bbox-polygons.ply";
  std::ofstream bbox_out(bbox_filename);
  bbox_out.precision(20);
  CGAL::write_PLY(bbox_out, bbox_mesh);
  bbox_out.close();

#endif
}

template<typename DS>
void dump_polygon_borders(const DS& data, const std::string tag = std::string()) {

  const std::string filename = (tag != std::string() ? tag + "-" : "") + "polygon-borders.polylines.txt";
  std::ofstream out(filename);
  out.precision(20);
  for (std::size_t i = 6; i < data.number_of_support_planes(); ++i) {
    for (const auto pedge : data.pedges(i)) {
      out << "2 " << data.segment_3(pedge) << std::endl;
    }
  }
  out.close();
}

template<typename DS, typename Event>
void dump_event(const DS& data, const Event& ev, const std::string tag = std::string()) {

  if (ev.is_pvertex_to_pvertex()) {

    const std::string vfilename = (tag != std::string() ? tag + "-" : "") + "event-pvertex.xyz";
    std::ofstream vout(vfilename);
    vout.precision(20);
    vout << data.point_3(ev.pvertex()) << std::endl;
    vout.close();

    const std::string ofilename = (tag != std::string() ? tag + "-" : "") + "event-pother.xyz";
    std::ofstream oout(ofilename);
    oout.precision(20);
    oout << data.point_3(ev.pother()) << std::endl;
    oout.close();

  } else if (ev.is_pvertex_to_iedge()) {

    const std::string lfilename = (tag != std::string() ? tag + "-" : "") + "event-iedge.polylines.txt";
    std::ofstream lout(lfilename);
    lout.precision(20);
    lout << "2 " << data.segment_3(ev.iedge()) << std::endl;
    lout.close();

    const std::string vfilename = (tag != std::string() ? tag + "-" : "") + "event-pvertex.xyz";
    std::ofstream vout(vfilename);
    vout.precision(20);
    vout << data.point_3(ev.pvertex());
    vout.close();

  } else if (ev.is_pvertex_to_ivertex()) {

    const std::string vfilename = (tag != std::string() ? tag + "-" : "") + "event-pvertex.xyz";
    std::ofstream vout(vfilename);
    vout.precision(20);
    vout << data.point_3(ev.pvertex()) << std::endl;
    vout.close();

    const std::string ofilename = (tag != std::string() ? tag + "-" : "") + "event-ivertex.xyz";
    std::ofstream oout(ofilename);
    oout.precision(20);
    oout << data.point_3(ev.ivertex()) << std::endl;
    oout.close();
  }
}

template<typename DS>
void dump(const DS& data, const std::string tag = std::string()) {

  dump_polygons(data, tag);
  dump_intersection_edges(data, tag);

  // dump_polygon_borders(data, tag);
  // dump_constrained_edges(data, tag);
}

template<typename GeomTraits>
class Saver {

public:
  using Traits    = GeomTraits;
  using FT        = typename Traits::FT;
  using Point_2   = typename Traits::Point_2;
  using Point_3   = typename Traits::Point_3;
  using Segment_2 = typename Traits::Segment_2;
  using Segment_3 = typename Traits::Segment_3;

  using Color        = CGAL::Color;
  using Surface_mesh = CGAL::Surface_mesh<Point_3>;
  using Random       = CGAL::Random;

  Saver() :
  m_path_prefix(""),
  grey(Color(125, 125, 125)),
  red(Color(125, 0, 0))
  { }

  void initialize(std::stringstream& stream) const {
    stream.precision(20);
  }

  void export_points_2(
    const std::vector<Point_2>& points,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    for (const auto& point : points)
      stream << point << " 0 " << std::endl;
    save(stream, file_name + ".xyz");
  }

  void export_points_2(
    const std::vector< std::vector<Point_2> >& regions,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    std::size_t num_points = 0;
    for (const auto& region : regions)
      num_points += region.size();
    add_ply_header_points(stream, num_points);

    for (std::size_t i = 0; i < regions.size(); ++i) {
      const auto color = get_idx_color(i);
      for (const auto& point : regions[i])
        stream << point << " 0 " << color << std::endl;
    }
    save(stream, file_name + ".ply");
  }

  void export_points_3(
    const std::vector<Point_3>& points,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    for (const auto& point : points)
      stream << point << std::endl;
    save(stream, file_name + ".xyz");
  }

  void export_segments_2(
    const std::vector<Segment_2>& segments,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    for (const auto& segment : segments)
      stream << "2 " << segment.source() << " 0 " << segment.target() << " 0 " << std::endl;
    save(stream, file_name + ".polylines.txt");
  }

  void export_segments_3(
    const std::vector<Segment_3>& segments,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    for (const auto& segment : segments)
      stream << "2 " << segment.source() << " " << segment.target() << std::endl;
    save(stream, file_name + ".polylines.txt");
  }

  void export_polygon_soup_3(
    const std::vector< std::vector<Point_3> >& polygons,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    std::size_t num_vertices = 0;
    for (const auto& polygon : polygons)
      num_vertices += polygon.size();
    std::size_t num_faces = polygons.size();
    add_ply_header_mesh(stream, num_vertices, num_faces);

    for (const auto& polygon : polygons)
      for (const auto& p : polygon)
        stream << p << std::endl;

    std::size_t i = 0, polygon_id = 0;
    for (const auto& polygon : polygons) {
      stream << polygon.size() << " ";
      for (std::size_t j = 0; j < polygon.size(); ++j)
        stream << i++ << " ";
      stream << get_idx_color(polygon_id) << std::endl;
      ++polygon_id;
    }
    save(stream, file_name + ".ply");
  }

  void export_polygon_soup_3(
    const std::vector< std::vector<Point_3> >& polygons,
    const std::vector<Color>& colors,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    std::size_t num_vertices = 0;
    for (const auto& polygon : polygons)
      num_vertices += polygon.size();
    std::size_t num_faces = polygons.size();
    add_ply_header_mesh(stream, num_vertices, num_faces);

    for (const auto& polygon : polygons)
      for (const auto& p : polygon)
        stream << p << std::endl;

    std::size_t i = 0, polygon_id = 0;
    for (std::size_t k = 0; k < polygons.size(); ++k) {
      stream << polygons[k].size() << " ";
      for (std::size_t j = 0; j < polygons[k].size(); ++j)
        stream << i++ << " ";
      stream << colors[k] << std::endl;
      ++polygon_id;
    }
    save(stream, file_name + ".ply");
  }

  void export_bounding_box_3(
    const std::array<Point_3, 8>& bounding_box,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    Surface_mesh bbox;
    CGAL_assertion(bounding_box.size() == 8);
    CGAL::make_hexahedron(
      bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3],
      bounding_box[4], bounding_box[5], bounding_box[6], bounding_box[7], bbox);
    stream << bbox;
    save(stream, file_name + ".off");
  }

  void export_mesh_2(
    const std::vector<Point_2>& vertices,
    const std::vector< std::vector<std::size_t> >& faces,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    add_ply_header_mesh(stream, vertices.size(), faces.size());
    for (const auto& vertex : vertices)
      stream << vertex << " 0 " << std::endl;

    for (const auto& face : faces) {
      stream << face.size();
      for (const std::size_t findex : face){
        stream << " " << findex;
      }
      stream << " " << grey << std::endl;
    }
    save(stream, file_name + ".ply");
  }

  void export_mesh_2(
    const std::vector<Point_2>& vertices,
    const std::vector< std::vector<std::size_t> >& faces,
    const std::vector<Color>& colors,
    const std::string file_name) const {

    std::stringstream stream;
    initialize(stream);

    add_ply_header_mesh(stream, vertices.size(), faces.size());
    for (const auto& vertex : vertices)
      stream << vertex << " 0 " << std::endl;

    for (std::size_t k = 0; k < faces.size(); ++k) {
      stream << faces[k].size();
      for (const std::size_t findex : faces[k]){
        stream << " " << findex;
      }
      stream << " " << colors[k] << std::endl;
    }
    save(stream, file_name + ".ply");
  }

  const Color get_idx_color(const std::size_t idx) const {
    Random rand(idx);
    const unsigned char r = rand.get_int(32, 192);
    const unsigned char g = rand.get_int(32, 192);
    const unsigned char b = rand.get_int(32, 192);
    return Color(r, g, b);
  }

private:
  const std::string m_path_prefix;
  const Color grey, red;

  void save(
    const std::stringstream& stream,
    const std::string file_name) const {

    const std::string file_path = m_path_prefix + file_name;
    std::ofstream file(file_path.c_str(), std::ios_base::out);

    if (!file)
      std::cerr << std::endl <<
        "ERROR: WHILE SAVING FILE " << file_path
      << "!" << std::endl << std::endl;

    file << stream.str();
    file.close();
  }

  void add_ply_header_points(
    std::stringstream& stream,
    const std::size_t size) const {

    stream <<
    "ply"                  +  std::string(_NL_) + ""                      <<
    "format ascii 1.0"     +  std::string(_NL_) + ""                      <<
    "element vertex "      << size        << "" + std::string(_NL_) + "" <<
    "property double x"    +  std::string(_NL_) + ""                     <<
    "property double y"    +  std::string(_NL_) + ""                     <<
    "property double z"    +  std::string(_NL_) + ""                      <<
    "property uchar red"   +  std::string(_NL_) + ""                      <<
    "property uchar green" +  std::string(_NL_) + ""                      <<
    "property uchar blue"  +  std::string(_NL_) + ""                      <<
    "property uchar alpha" +  std::string(_NL_) + ""                      <<
    "end_header"           +  std::string(_NL_) + "";
  }

  void add_ply_header_normals(
    std::stringstream& stream,
    const std::size_t size) const {

    stream <<
    "ply"                  +  std::string(_NL_) + ""                      <<
    "format ascii 1.0"     +  std::string(_NL_) + ""                      <<
    "element vertex "      << size        << "" + std::string(_NL_) + "" <<
    "property double x"    +  std::string(_NL_) + ""                     <<
    "property double y"    +  std::string(_NL_) + ""                     <<
    "property double z"    +  std::string(_NL_) + ""                      <<
    "property double nx"   +  std::string(_NL_) + ""                     <<
    "property double ny"   +  std::string(_NL_) + ""                     <<
    "property double nz"   +  std::string(_NL_) + ""                      <<
    "end_header"           +  std::string(_NL_) + "";
  }

  void add_ply_header_mesh(
    std::stringstream& stream,
    const std::size_t num_vertices,
    const std::size_t num_faces) const {

    stream <<
    "ply"                  +  std::string(_NL_) + ""                     <<
    "format ascii 1.0"     +  std::string(_NL_) + ""                     <<
    "element vertex "      << num_vertices     << "" + std::string(_NL_) + "" <<
    "property double x"    +  std::string(_NL_) + ""                    <<
    "property double y"    +  std::string(_NL_) + ""                    <<
    "property double z"    +  std::string(_NL_) + ""                     <<
    "element face "        << num_faces        << "" + std::string(_NL_) + "" <<
    "property list uchar int vertex_indices"         + std::string(_NL_) + "" <<
    "property uchar red"   +  std::string(_NL_) + ""                     <<
    "property uchar green" +  std::string(_NL_) + ""                     <<
    "property uchar blue"  +  std::string(_NL_) + ""                     <<
    "property uchar alpha" +  std::string(_NL_) + ""                      <<
    "end_header"           +  std::string(_NL_) + "";
  }
};

template<typename DS, typename PFace>
void dump_volume(
  const DS& data,
  const std::vector<PFace>& pfaces,
  const std::string file_name,
  const bool use_colors = true) {

  using Point_3 = typename DS::Kernel::Point_3;
  std::vector<Point_3> polygon;
  std::vector< std::vector<Point_3> > polygons;
  std::vector<Color> colors;

  colors.reserve(pfaces.size());
  polygons.reserve(pfaces.size());

  Saver<typename DS::Kernel> saver;
  for (const auto& pface : pfaces) {
    const auto pvertices = data.pvertices_of_pface(pface);
    const auto color = saver.get_idx_color(static_cast<int>(pface.second));
    polygon.clear();
    for (const auto pvertex : pvertices) {
      polygon.push_back(data.point_3(pvertex));
    }
    if (use_colors) {
      colors.push_back(color);
    } else {
      colors.push_back(saver.get_idx_color(0));
    }
    CGAL_assertion(polygon.size() >= 3);
    polygons.push_back(polygon);
  }
  CGAL_assertion(colors.size() == pfaces.size());
  CGAL_assertion(polygons.size() == pfaces.size());

  saver.export_polygon_soup_3(polygons, colors, file_name);
}

template<typename DS>
void dump_volumes(const DS& data, const std::string tag = std::string()) {

  using Point_3 = typename DS::Kernel::Point_3;
  std::vector<Point_3> polygon;
  std::vector< std::vector<Point_3> > polygons;
  std::vector<Color> colors;

  Saver<typename DS::Kernel> saver;
  for (std::size_t i = 0; i < data.volumes().size(); ++i) {
    const auto& volume = data.volumes()[i];
    const auto color = saver.get_idx_color(i);

    colors.clear();
    polygons.clear();
    for (const auto& pface : volume.pfaces) {
      polygon.clear();
      for (const auto pvertex : data.pvertices_of_pface(pface)) {
        polygon.push_back(data.point_3(pvertex));
      }
      CGAL_assertion(polygon.size() >= 3);
      polygons.push_back(polygon);
      colors.push_back(color);
    }

    const std::string file_name =
      (tag != std::string() ? tag + "-" : "") + "volume-" + std::to_string(i);
    saver.export_polygon_soup_3(polygons, colors, file_name);
  }
}

template<typename DS, typename Polygon_2>
void dump_polygon(
  const DS& data,
  const std::size_t sp_idx,
  const Polygon_2& input,
  const std::string name) {

  using Kernel = typename DS::Kernel;
  using Point_3 = typename Kernel::Point_3;
  std::vector<Point_3> polygon;
  std::vector< std::vector<Point_3> > polygons;
  for (const auto& point_2 : input) {
    polygon.push_back(data.to_3d(sp_idx, point_2));
  }
  polygons.push_back(polygon);
  Saver<Kernel> saver;
  saver.export_polygon_soup_3(polygons, "volumes/" + name);
}

template<typename DS, typename PFace>
void dump_pface(
  const DS& data,
  const PFace& pface,
  const std::string name) {

  using Kernel = typename DS::Kernel;
  using Point_3 = typename Kernel::Point_3;
  std::vector<Point_3> polygon;
  std::vector< std::vector<Point_3> > polygons;
  for (const auto pvertex : data.pvertices_of_pface(pface)) {
    polygon.push_back(data.point_3(pvertex));
  }
  CGAL_assertion(polygon.size() >= 3);
  polygons.push_back(polygon);
  Saver<Kernel> saver;
  saver.export_polygon_soup_3(polygons, "volumes/" + name);
}

template<typename DS, typename PEdge>
void dump_pedge(
  const DS& data,
  const PEdge& pedge,
  const std::string name) {

  using Kernel = typename DS::Kernel;
  using Segment_3 = typename Kernel::Segment_3;
  const std::vector<Segment_3> segments = { data.segment_3(pedge) };
  Saver<Kernel> saver;
  saver.export_segments_3(segments, "volumes/" + name);
}

template<typename DS, typename PFace, typename PEdge>
void dump_info(
  const DS& data,
  const PFace& pface,
  const PEdge& pedge,
  const std::vector<PFace>& nfaces) {

  std::cout << "DEBUG: number of found nfaces: " << nfaces.size() << std::endl;
  dump_pface(data, pface, "face-curr");
  dump_pedge(data, pedge, "face-edge");
  for (std::size_t i = 0; i < nfaces.size(); ++i) {
    dump_pface(data, nfaces[i], "nface-" + std::to_string(i));
  }
}

template<typename Point_3>
void dump_frame(
  const std::vector<Point_3>& points,
  const std::string name) {

  using Kernel = typename Kernel_traits<Point_3>::Kernel;
  using Segment_3 = typename Kernel::Segment_3;
  std::vector<Segment_3> segments;
  segments.reserve(points.size() - 1);
  for (std::size_t i = 1; i < points.size(); ++i)
    segments.push_back(Segment_3(points[0], points[i]));
  Saver<Kernel> saver;
  saver.export_segments_3(segments, name);
}

template<typename DS, typename CDT>
void dump_cdt(
  const DS& data, const std::size_t sp_idx, const CDT& cdt, std::string file_name) {

  using Point_3       = typename DS::Kernel::Point_3;
  using Vertex_handle = typename CDT::Vertex_handle;

  using Mesh_3 = CGAL::Surface_mesh<Point_3>;
  using VIdx   = typename Mesh_3::Vertex_index;
  using FIdx   = typename Mesh_3::Face_index;
  using UM     = typename Mesh_3::template Property_map<FIdx, unsigned char>;

  Mesh_3 mesh;
  UM red   = mesh.template add_property_map<FIdx, unsigned char>("red"  , 125).first;
  UM green = mesh.template add_property_map<FIdx, unsigned char>("green", 125).first;
  UM blue  = mesh.template add_property_map<FIdx, unsigned char>("blue" , 125).first;

  std::map<Vertex_handle, VIdx> map_v2i;
  for (auto vit = cdt.finite_vertices_begin(); vit != cdt.finite_vertices_end(); ++vit) {
    const auto& ipoint = vit->point();
    map_v2i.insert(std::make_pair(
      vit, mesh.add_vertex(data.support_plane(sp_idx).to_3d(ipoint))));
  }

  for (auto fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) {
    std::array<VIdx, 3> vertices;
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

  file_name += "support-cdt-" + std::to_string(sp_idx) + ".ply";
  std::ofstream out(file_name);
  out.precision(20);
  CGAL::IO::write_PLY(out, mesh);
  out.close();
}

} // namespace KSR_3
} // namespace CGAL

#endif // CGAL_KSR_DEBUG_H
