//TODO: add for soup face the id of the input face. not sure it is easy to report intersection edge as a pair of vertex id
//TODO: only return intersection segments
// Copyright (c) 2023 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Sebastien Loriot
//

#ifndef CGAL_POLYGON_MESH_PROCESSING_AUTOREFINEMENT_H
#define CGAL_POLYGON_MESH_PROCESSING_AUTOREFINEMENT_H

#include <CGAL/license/Polygon_mesh_processing/geometric_repair.h>

#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/shape_predicates.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_plus_2.h>
#include <CGAL/Projection_traits_3.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

// output
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/polygon_mesh_to_polygon_soup.h>

#ifndef CGAL_PMP_AUTOREFINE_VERBOSE
#define CGAL_PMP_AUTOREFINE_VERBOSE(MSG)
#endif

#include <vector>

namespace CGAL {
namespace Polygon_mesh_processing {

#ifndef DOXYGEN_RUNNING
namespace autorefine_impl {

template <class EK>
void generate_subtriangles(std::size_t ti,
                                 std::vector<typename EK::Segment_3>& segments,
                           const std::vector<typename EK::Point_3>& points,
                           const std::vector<std::size_t>& in_triangle_ids,
                           const std::set<std::pair<std::size_t, std::size_t> >& intersecting_triangles,
                           const std::vector<typename EK::Triangle_3>& triangles,
                           std::vector<typename EK::Triangle_3>& new_triangles)
{
  typedef CGAL::Projection_traits_3<EK> P_traits;
  typedef CGAL::Exact_intersections_tag Itag;

  typedef CGAL::Constrained_Delaunay_triangulation_2<P_traits/* , Default, Itag */> CDT_2;
  //typedef CGAL::Constrained_triangulation_plus_2<CDT_base> CDT;
  typedef CDT_2 CDT;

  const typename EK::Triangle_3& t = triangles[ti];

  // positive triangle normal
  typename EK::Vector_3 n = normal(t[0], t[1], t[2]);
  typename EK::Point_3 o(CGAL::ORIGIN);

  bool orientation_flipped = false;
  if ( typename EK::Less_xyz_3()(o+n,o) )
  {
    n=-n;
    orientation_flipped = true;
  }

  P_traits cdt_traits(n);
  CDT cdt(cdt_traits);

  cdt.insert_outside_affine_hull(t[0]);
  cdt.insert_outside_affine_hull(t[1]);
  typename CDT::Vertex_handle v = cdt.tds().insert_dim_up(cdt.infinite_vertex(), orientation_flipped);
  v->set_point(t[2]);

#if 1
  //~ static std::ofstream debug("inter_segments.polylines.txt");
  //~ debug.precision(17);

  // pre-compute segment intersections
  if (!segments.empty())
  {
    std::size_t nbs = segments.size();
    //~ std::cout << "nbs " << nbs << "\n";

    //~ if (nbs==8)
    //~ {
      //~ for (std::size_t i = 0; i<nbs; ++i)
        //~ std::ofstream("cst_"+std::to_string(i)+".polylines.txt") << std::setprecision(17) << "2 " << segments[i] << "\n";
    //~ }

    std::vector< std::vector<typename EK::Point_3> > points_on_segments(nbs);
    for (std::size_t i = 0; i<nbs-1; ++i)
    {
      for (std::size_t j = i+1; j<nbs; ++j)
      {
        if (intersecting_triangles.count(CGAL::make_sorted_pair(in_triangle_ids[i], in_triangle_ids[j]))!=0)
        {
          if (CGAL::do_intersect(segments[i], segments[j]))
          {
            auto res = CGAL::intersection(triangles[in_triangle_ids[i]].supporting_plane(),
                                          triangles[in_triangle_ids[j]].supporting_plane(),
                                          triangles[ti].supporting_plane());

            if (const typename EK::Point_3* pt_ptr = boost::get<typename EK::Point_3>(&(*res)))
            {
              points_on_segments[i].push_back(*pt_ptr);
              points_on_segments[j].push_back(*pt_ptr);

              //~ std::cout << "new inter " << *pt_ptr << "\n";

            }
            else
            {
              // We can have hard cases if two triangles are coplanar....

              //~ std::cout << "coplanar inter: " << i << " " << j << "\n";

              auto inter = CGAL::intersection(segments[i], segments[j]);
              if (const typename EK::Point_3* pt_ptr = boost::get<typename EK::Point_3>(&(*inter)))
              {
                points_on_segments[i].push_back(*pt_ptr);
                points_on_segments[j].push_back(*pt_ptr);

                //~ std::cout << "new inter bis" << *pt_ptr << "\n";
              }
              else
              {
                if (const typename EK::Segment_3* seg_ptr = boost::get<typename EK::Segment_3>(&(*inter)))
                {
                  points_on_segments[i].push_back(seg_ptr->source());
                  points_on_segments[j].push_back(seg_ptr->source());
                  points_on_segments[i].push_back(seg_ptr->target());
                  points_on_segments[j].push_back(seg_ptr->target());

                }
                else
                  std::cerr <<"ERROR!\n";
              }

              #if 0
              //this code works if triangles are not coplanar
              // coplanar intersection that is not a point
              int coord = 0;
              const typename EK::Segment_3& s = segments[i];
              typename EK::Point_3 src = s.source(), tgt=s.target();
              if (src.x()==tgt.x())
              {
                coord=1;
                if (src.y()==tgt.y())
                  coord==2;
              }

              std::vector<typename EK::Point_3> tmp_pts = {
                src, tgt, segments[j][0], segments[j][1] };

              std::sort(tmp_pts.begin(), tmp_pts.end(),
                        [coord](const typename EK::Point_3& p, const typename EK::Point_3& q)
                        {return p[coord]<q[coord];});

              points_on_segments[i].push_back(tmp_pts[1]);
              points_on_segments[i].push_back(tmp_pts[2]);
              points_on_segments[j].push_back(tmp_pts[1]);
              points_on_segments[j].push_back(tmp_pts[2]);
#endif
              //~ std::cout << "new inter coli " << segments[j].source() << "\n";
              //~ std::cout << "new inter coli " << segments[j].target() << "\n";
              //~ std::cout << "new inter coli " << segments[i].source() << "\n";
              //~ std::cout << "new inter coli " << segments[i].target() << "\n";

              //~ points_on_segments[j].push_back(*pt_ptr);



              //~ std::cerr << "ERROR: intersection is a segment\n";
              //~ std::cout << std::setprecision(17);
              //~ exact(segments[i]);
              //~ exact(segments[j]);
              //~ std::cout << segments[i] << "\n";
              //~ std::cout << segments[j] << "\n";
              //~ debug << "4 " << triangles[in_triangle_ids[i]] << " " << triangles[in_triangle_ids[i]][0] << "\n";
              //~ debug << "4 " << triangles[in_triangle_ids[j]] << " " << triangles[in_triangle_ids[j]][0] << "\n";
              //~ debug << "4 " << triangles[ti] << " " << triangles[ti][0] << "\n";
              //~ exit(1);
            }
          }
        }
      }
    }

    std::vector<typename EK::Point_3> cst_points;
    std::vector<std::pair<std::size_t, std::size_t>> csts;
    for (std::size_t i = 0; i<nbs; ++i)
    {
      if(!points_on_segments[i].empty())
      {
        // TODO: predicate on input triangles
        int coord = 0;
        const typename EK::Segment_3& s = segments[i];
        typename EK::Point_3 src = s.source(), tgt=s.target();
        if (src.x()==tgt.x())
        {
          coord=1;
          if (src.y()==tgt.y())
            coord==2;
        }
        if (src[coord]>tgt[coord])
          std::swap(src, tgt);

        std::sort(points_on_segments[i].begin(), points_on_segments[i].end(), [coord](const typename EK::Point_3& p, const typename EK::Point_3& q){return p[coord]<q[coord];});
//TODO: use reserve on cst_points?
        std::size_t src_id=cst_points.size();
        cst_points.push_back(src);
        cst_points.insert(cst_points.end(), points_on_segments[i].begin(), points_on_segments[i].end());
        cst_points.push_back(tgt);


        //~ {
        //~ std::cout << "cst_points.size() " << cst_points.size() << "\n";
        //~ std::ofstream debugbis("subcst.polylines.txt");
        //~ debugbis << std::setprecision(17);

        //~ std::cout << "splittng " << segments[i] << "\n";
        //~ static int kkk=-1;
        //~ ++kkk;
        //~ std::ofstream debugter("subcst_"+std::to_string(i)+".polylines.txt");
        //~ for (std::size_t k=0; k<=points_on_segments[i].size(); ++k)
        //~ {
          //~ exact(cst_points[src_id+k]);
          //~ exact(cst_points[src_id+k+1]);
          //~ debugbis << "2 " << cst_points[src_id+k] << " " <<  cst_points[src_id+k+1] << "\n";
          //~ debugter << "2 " << cst_points[src_id+k] << " " <<  cst_points[src_id+k+1] << "\n";
        //~ }
        //~ std::cout << "---\n";
        //~ }

        for (std::size_t k=0; k<=points_on_segments[i].size(); ++k)
          csts.emplace_back(src_id+k, src_id+k+1);
      }
    }

    cdt.insert_constraints(cst_points.begin(), cst_points.end(), csts.begin(), csts.end());

    std::vector<typename EK::Segment_3> no_inter_segments;
    no_inter_segments.reserve(nbs);
    for (std::size_t i = 0; i<nbs; ++i)
      if(points_on_segments[i].empty())
        no_inter_segments.push_back(segments[i]);
    no_inter_segments.swap(segments);
  }
  //~ std::cout << "done\n";
#endif

  cdt.insert_constraints(segments.begin(), segments.end());
  cdt.insert(points.begin(), points.end());

#ifdef CGAL_DEBUG_PMP_AUTOREFINE_DUMP_TRIANGULATIONS
    static int k = 0;
    std::stringstream buffer;
    buffer.precision(17);
    int nbt=0;
#endif
    for (typename CDT::Face_handle fh : cdt.finite_face_handles())
    {
      if (orientation_flipped)
        new_triangles.emplace_back(fh->vertex(0)->point(),
                                   fh->vertex(cdt.cw(0))->point(),
                                   fh->vertex(cdt.ccw(0))->point());
      else
        new_triangles.emplace_back(fh->vertex(0)->point(),
                                   fh->vertex(cdt.ccw(0))->point(),
                                   fh->vertex(cdt.cw(0))->point());
#ifdef CGAL_DEBUG_PMP_AUTOREFINE_DUMP_TRIANGULATIONS
      ++nbt;
      buffer << fh->vertex(0)->point() << "\n";
      buffer << fh->vertex(cdt.ccw(0))->point() << "\n";
      buffer << fh->vertex(cdt.cw(0))->point() << "\n";
#endif
    }

#ifdef CGAL_DEBUG_PMP_AUTOREFINE_DUMP_TRIANGULATIONS
    std::ofstream dump("triangulation_"+std::to_string(k)+".off");
    dump << "OFF\n" << 3*nbt << " " << nbt << " 0\n";
    dump << buffer.str();
    for (int i=0; i<nbt; ++i)
      dump << "3 " << 3*i << " " << 3*i+1 << " " << 3*i+2 << "\n";
    ++k;
#endif
}

template <class EK>
struct Intersection_visitor
{
  std::vector< std::vector<typename EK::Segment_3> >& all_segments;
  std::vector< std::vector<typename EK::Point_3> >& all_points;
  std::vector< std::vector<std::size_t> >& all_in_triangle_ids;
  std::pair<int, int> ids;

  Intersection_visitor(std::vector< std::vector<typename EK::Segment_3> >& all_segments,
                       std::vector< std::vector<typename EK::Point_3> >& all_points,
                       std::vector< std::vector<std::size_t> >& all_in_triangle_ids)
    : all_segments (all_segments)
    , all_points(all_points)
    , all_in_triangle_ids(all_in_triangle_ids)
  {}

  void set_triangle_ids(int i1, int i2)
  {
    ids = {i1, i2};
  }

  typedef void result_type;
  void operator()(const typename EK::Point_3& p)
  {
    all_points[ids.first].push_back(p);
    all_points[ids.second].push_back(p);
  }

  void operator()(const typename EK::Segment_3& s)
  {
    all_segments[ids.first].push_back(s);
    all_segments[ids.second].push_back(s);
    all_in_triangle_ids[ids.first].push_back(ids.second);
    all_in_triangle_ids[ids.second].push_back(ids.first);
  }

  void operator()(const typename EK::Triangle_3& t)
  {
    for (std::size_t i=0; i<3; ++i)
    {
      typename EK::Segment_3 s(t[i], t[(i+1)%3]);
      all_segments[ids.first].push_back(s);
      all_segments[ids.second].push_back(s);
      all_in_triangle_ids[ids.first].push_back(ids.second);
      all_in_triangle_ids[ids.second].push_back(ids.first);
    }

  }

  void operator()(const std::vector<typename EK::Point_3>& poly)
  {
    std::size_t nbp = poly.size();
    for (std::size_t i=0; i<nbp; ++i)
    {
      typename EK::Segment_3 s(poly[i], poly[(i+1)%nbp]);
      all_segments[ids.first].push_back(s);
      all_segments[ids.second].push_back(s);
      all_in_triangle_ids[ids.first].push_back(ids.second);
      all_in_triangle_ids[ids.second].push_back(ids.first);
    }
  }
};

} // end of autorefine_impl

template <class PointRange, class TriIdsRange, class Point_3, class NamedParameters = parameters::Default_named_parameters>
void autorefine_soup_output(const PointRange& input_points,
                            const TriIdsRange& id_triples,
                            std::vector<Point_3>& soup_points,
                            std::vector<std::array<std::size_t, 3> >& soup_triangles,
                            const NamedParameters& np = parameters::default_values())
{
  using parameters::choose_parameter;
  using parameters::get_parameter;

  typedef typename GetPolygonSoupGeomTraits<PointRange, NamedParameters>::type GT;
  typedef typename GetPointMap<PointRange, NamedParameters>::const_type    Point_map;
  Point_map pm = choose_parameter<Point_map>(get_parameter(np, internal_np::point_map));

  typedef typename internal_np::Lookup_named_param_def <
    internal_np::concurrency_tag_t,
    NamedParameters,
    Sequential_tag
  > ::type Concurrency_tag;

  typedef std::size_t Input_TID;
  typedef std::pair<Input_TID, Input_TID> Pair_of_triangle_ids;

  std::vector<Pair_of_triangle_ids> si_pairs;

  // collect intersecting pairs of triangles
  CGAL_PMP_AUTOREFINE_VERBOSE("collect intersecting pairs");
  triangle_soup_self_intersections<Concurrency_tag>(input_points, id_triples, std::back_inserter(si_pairs), np);

  if (si_pairs.empty()) return;

  // mark degenerate faces so that we can ignore them
  std::vector<bool> is_degen(id_triples.size(), false);

  for (const Pair_of_triangle_ids& p : si_pairs)
    if (p.first==p.second) // bbox inter reports (f,f) for degenerate faces
      is_degen[p.first] = true;

  // assign an id per triangle involved in an intersection
  // + the faces involved in the intersection
  std::vector<int> tri_inter_ids(id_triples.size(), -1);
  std::vector<Input_TID> intersected_faces;
  int tiid=-1;
  for (const Pair_of_triangle_ids& p : si_pairs)
  {
    if (tri_inter_ids[p.first]==-1 && !is_degen[p.first])
    {
      tri_inter_ids[p.first]=++tiid;
      intersected_faces.push_back(p.first);
    }
    if (tri_inter_ids[p.second]==-1 && !is_degen[p.second])
    {
      tri_inter_ids[p.second]=++tiid;
      intersected_faces.push_back(p.second);
    }
  }

  // init the vector of triangles used for the autorefinement of triangles
  typedef CGAL::Exact_predicates_exact_constructions_kernel EK;
  std::vector< EK::Triangle_3 > triangles(tiid+1);
  Cartesian_converter<GT, EK> to_exact;

  for(Input_TID f : intersected_faces)
  {
    triangles[tri_inter_ids[f]]= EK::Triangle_3(
      to_exact( get(pm, input_points[id_triples[f][0]]) ),
      to_exact( get(pm, input_points[id_triples[f][1]]) ),
      to_exact( get(pm, input_points[id_triples[f][2]]) ) );
  }

  std::vector< std::vector<EK::Segment_3> > all_segments(triangles.size());
  std::vector< std::vector<EK::Point_3> > all_points(triangles.size());
  std::vector< std::vector<std::size_t> > all_in_triangle_ids(triangles.size());

  CGAL_PMP_AUTOREFINE_VERBOSE("compute intersections");
  typename EK::Intersect_3 intersection = EK().intersect_3_object();
  autorefine_impl::Intersection_visitor<EK> intersection_visitor(all_segments, all_points, all_in_triangle_ids);

  std::set<std::pair<std::size_t, std::size_t> > intersecting_triangles;
  for (const Pair_of_triangle_ids& p : si_pairs)
  {
    int i1 = tri_inter_ids[p.first],
        i2 = tri_inter_ids[p.second];

    if (i1==-1 || i2==-1) continue; //skip degenerate faces

    const EK::Triangle_3& t1 = triangles[i1];
    const EK::Triangle_3& t2 = triangles[i2];

    auto inter = intersection(t1, t2);

    if (inter != boost::none)
    {
      intersecting_triangles.insert(CGAL::make_sorted_pair(i1, i2));
      intersection_visitor.set_triangle_ids(i1, i2);
      boost::apply_visitor(intersection_visitor, *inter);
    }
  }

  // deduplicate inserted segments
  Cartesian_converter<EK, GT> to_input;
  std::map<EK::Point_3, std::size_t> point_id_map;
#if ! defined(CGAL_NDEBUG) || defined(CGAL_DEBUG_PMP_AUTOREFINE)
  std::vector<EK::Point_3> exact_soup_points;
#endif

  auto get_point_id = [&](const typename EK::Point_3& pt)
  {
    auto insert_res = point_id_map.insert(std::make_pair(pt, soup_points.size()));
    if (insert_res.second)
    {
      soup_points.push_back(to_input(pt));
#if ! defined(CGAL_NDEBUG) || defined(CGAL_DEBUG_PMP_AUTOREFINE)
      exact_soup_points.push_back(pt);
#endif
    }
    return insert_res.first->second;
  };

  // filter duplicated segments
  for(std::size_t ti=0; ti<triangles.size(); ++ti)
  {
    if (!all_segments[ti].empty())
    {
      std::size_t nbs = all_segments[ti].size();
      std::vector<EK::Segment_3> filtered_segments;
      std::vector<std::size_t> filtered_in_triangle_ids;
      filtered_segments.reserve(nbs);
      std::set<std::pair<std::size_t, std::size_t>> segset;
      for (std::size_t si=0; si<nbs; ++si)
      {
        EK::Point_3 src = all_segments[ti][si].source(),
                    tgt = all_segments[ti][si].target();
        if (segset.insert(
              CGAL::make_sorted_pair( get_point_id(src),
                                      get_point_id(tgt))).second)
        {
          filtered_segments.push_back(all_segments[ti][si]);
          filtered_in_triangle_ids.push_back(all_in_triangle_ids[ti][si]);
        }
      }
      if (filtered_segments.size()!=nbs)
      {
        filtered_segments.swap(all_segments[ti]);
        filtered_in_triangle_ids.swap(all_in_triangle_ids[ti]);
      }
    }
  }

  CGAL_PMP_AUTOREFINE_VERBOSE("triangulate faces");
  // now refine triangles
  std::vector<EK::Triangle_3> new_triangles;
  for(std::size_t ti=0; ti<triangles.size(); ++ti)
  {
    if (all_segments[ti].empty() && all_points[ti].empty())
      new_triangles.push_back(triangles[ti]);
    else
      autorefine_impl::generate_subtriangles<EK>(ti, all_segments[ti], all_points[ti], all_in_triangle_ids[ti], intersecting_triangles, triangles, new_triangles);
  }


  // brute force output: create a soup, orient and to-mesh
  CGAL_PMP_AUTOREFINE_VERBOSE("create output soup");

  std::vector <std::size_t> input_point_ids;
  input_point_ids.reserve(input_points.size());
  for (const auto& p : input_points)
    input_point_ids.push_back(get_point_id(to_exact(get(pm,p))));

  for (Input_TID f=0; f<id_triples.size(); ++f)
  {
    if (is_degen[f]) continue; //skip degenerate faces

    int tiid = tri_inter_ids[f];
    if (tiid == -1)
    {
      soup_triangles.emplace_back(
        CGAL::make_array(input_point_ids[id_triples[f][0]],
                         input_point_ids[id_triples[f][1]],
                         input_point_ids[id_triples[f][2]])
      );
    }
  }
  for (const typename EK::Triangle_3& t : new_triangles)
  {
    soup_triangles.emplace_back(CGAL::make_array(get_point_id(t[0]), get_point_id(t[1]), get_point_id(t[2])));
  }

#ifndef CGAL_NDEBUG
  CGAL_PMP_AUTOREFINE_VERBOSE("check soup");
  CGAL_assertion( !does_triangle_soup_self_intersect(exact_soup_points, soup_triangles) );
#else
#ifdef CGAL_DEBUG_PMP_AUTOREFINE
  CGAL_PMP_AUTOREFINE_VERBOSE("check soup");
  if (does_triangle_soup_self_intersect(exact_soup_points, soup_triangles))
    throw std::runtime_error("invalid output");
#endif
#endif
  CGAL_PMP_AUTOREFINE_VERBOSE("done");
}
#endif

/**
 * \ingroup PMP_corefinement_grp
 * refines a triangle mesh so that no triangles intersects in their interior.
 *
 * @tparam TriangleMesh a model of `HalfedgeListGraph`, `FaceListGraph`, and `MutableFaceGraph`
 * @tparam NamedParameters a sequence of \ref namedparameters
 *
 * @param tm input triangulated surface mesh
 * @param np an optional sequence of \ref bgl_namedparameters "Named Parameters" among the ones listed below
 *
 * \cgalParamNBegin{geom_traits}
 *   \cgalParamDescription{an instance of a geometric traits class}
 *   \cgalParamType{a class model of `PMPSelfIntersectionTraits`}
 *   \cgalParamDefault{a \cgal Kernel deduced from the point type, using `CGAL::Kernel_traits`}
 *   \cgalParamExtra{The geometric traits class must be compatible with the vertex point type.}
 * \cgalParamNEnd
 *
 * \cgalNamedParamsBegin
 *   \cgalParamNBegin{vertex_point_map}
 *     \cgalParamDescription{a property map associating points to the vertices of `tm`}
 *     \cgalParamType{a class model of `ReadablePropertyMap` with `boost::graph_traits<TriangleMesh>::%vertex_descriptor`
 *                    as key type and `%Point_3` as value type}
 *     \cgalParamDefault{`boost::get(CGAL::vertex_point, tm)`}
 *     \cgalParamExtra{If this parameter is omitted, an internal property map for `CGAL::vertex_point_t`
 *                     must be available in `TriangleMesh`.}
 *  \cgalParamNEnd
 *
 */
template <class TriangleMesh,
          class NamedParameters = parameters::Default_named_parameters>
void
autorefine(      TriangleMesh& tm,
           const NamedParameters& np = parameters::default_values())
{
  using parameters::choose_parameter;
  using parameters::get_parameter;

  typedef typename GetGeomTraits<TriangleMesh, NamedParameters>::type GT;
  GT traits = choose_parameter<GT>(get_parameter(np, internal_np::geom_traits));

  std::vector<typename GT::Point_3> in_soup_points;
  std::vector<std::array<std::size_t, 3> > in_soup_triangles;
  std::vector<typename GT::Point_3> out_soup_points;
  std::vector<std::array<std::size_t, 3> > out_soup_triangles;

  polygon_mesh_to_polygon_soup(tm, in_soup_points, in_soup_triangles);

  autorefine_soup_output(in_soup_points, in_soup_triangles,
                         out_soup_points, out_soup_triangles);

  clear(tm);
  orient_polygon_soup(out_soup_points, out_soup_triangles);
  polygon_soup_to_polygon_mesh(out_soup_points, out_soup_triangles, tm);
}


} } // end of CGAL::Polygon_mesh_processing

#endif // CGAL_POLYGON_MESH_PROCESSING_AUTOREFINEMENT_H
