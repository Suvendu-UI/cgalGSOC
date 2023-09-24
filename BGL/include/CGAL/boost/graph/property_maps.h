// Copyright (c) 2012 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)     : Sebastien Loriot


#ifndef CGAL_BOOST_GRAPH_PROPERTY_MAPS_H
#define CGAL_BOOST_GRAPH_PROPERTY_MAPS_H

#include <CGAL/property_map.h>
#include <CGAL/boost/graph/properties.h>
#include <boost/mpl/if.hpp>

namespace CGAL{

//property map
template < class TriangleMesh,
           class VertexPointMap = typename boost::property_map<TriangleMesh,vertex_point_t>::type >
struct Triangle_from_face_descriptor_map{
  typename std::remove_const_t<TriangleMesh>* m_tm;
  std::optional<VertexPointMap> m_vpm;

  Triangle_from_face_descriptor_map()
    : m_tm(nullptr), m_vpm()
  {}

  Triangle_from_face_descriptor_map(TriangleMesh const* tm)
    : m_tm( const_cast<std::remove_const_t<TriangleMesh>*>(tm) )
    , m_vpm( get(vertex_point, *m_tm) )
  {}

  Triangle_from_face_descriptor_map(TriangleMesh const* tm,
                                    VertexPointMap vpm )
    : m_tm(const_cast<std::remove_const_t<TriangleMesh>*>(tm))
    , m_vpm(vpm)
  {}

  typedef typename boost::property_traits< VertexPointMap >::value_type Point_3;

  //classical typedefs
  typedef typename boost::graph_traits<TriangleMesh>::face_descriptor key_type;
  typedef typename Kernel_traits<Point_3>::Kernel::Triangle_3 value_type;
  typedef value_type reference;
  typedef boost::readable_property_map_tag category;

  //get function for property map
  inline friend
  value_type
  get(const Triangle_from_face_descriptor_map<TriangleMesh,VertexPointMap>& pmap,
      key_type f)
  {
    std::remove_const_t<TriangleMesh>& tm = *(pmap.m_tm);
    CGAL_precondition(halfedge(f,tm) == next(next(next(halfedge(f,tm),tm),tm),tm));

    return value_type( get(pmap.m_vpm.value(), target(halfedge(f,tm),tm)),
                       get(pmap.m_vpm.value(), target(next(halfedge(f,tm),tm),tm)),
                       get(pmap.m_vpm.value(), source(halfedge(f,tm),tm)) );
  }

  inline friend
  value_type
  get(const Triangle_from_face_descriptor_map<TriangleMesh,VertexPointMap>& pmap,
      const std::pair<key_type, const TriangleMesh*>& f)
  {
    std::remove_const_t<TriangleMesh> & tm = *(pmap.m_tm);
    CGAL_precondition(halfedge(f.first,tm) == next(next(next(halfedge(f.first,tm),tm),tm),tm));

    return value_type( get(pmap.m_vpm.value(), target(halfedge(f.first,tm),tm)),
                       get(pmap.m_vpm.value(), target(next(halfedge(f.first,tm),tm),tm)),
                       get(pmap.m_vpm.value(), source(halfedge(f.first,tm),tm)) );
  }
};

template < class PolygonMesh,
           class VertexPointMap = typename boost::property_map<PolygonMesh,vertex_point_t>::type >
struct Segment_from_edge_descriptor_map{

  Segment_from_edge_descriptor_map()
    : m_pm(nullptr), m_vpm()
  {}

  Segment_from_edge_descriptor_map(PolygonMesh const * pm)
    : m_pm( const_cast<std::remove_const_t<PolygonMesh>*>(pm) )
    , m_vpm( get(vertex_point, *m_pm) )
  {}

  Segment_from_edge_descriptor_map(PolygonMesh const * pm,
                                   VertexPointMap vpm )
    : m_pm( const_cast<std::remove_const_t<PolygonMesh>*>(pm) )
    , m_vpm(vpm)
  {}

  typedef typename boost::property_traits< VertexPointMap >::value_type Point_3;

  //classical typedefs
  typedef typename boost::graph_traits<PolygonMesh>::edge_descriptor key_type;
  typedef typename Kernel_traits<Point_3>::Kernel::Segment_3 value_type;
  typedef value_type reference;
  typedef boost::readable_property_map_tag category;
  //data
  std::remove_const_t<PolygonMesh>* m_pm;
  std::optional<VertexPointMap> m_vpm;

  //get function for property map
  inline friend
  value_type
  get(const Segment_from_edge_descriptor_map<PolygonMesh,VertexPointMap>& pmap,
      key_type h)
  {
    return value_type(get(pmap.m_vpm.value(), source(h, *pmap.m_pm) ),
                      get(pmap.m_vpm.value(), target(h, *pmap.m_pm) ) );
  }

  inline friend
  value_type
  get(const Segment_from_edge_descriptor_map<PolygonMesh,VertexPointMap>& pmap,
      const std::pair<key_type, const PolygonMesh*>& h)
  {
    return value_type(get(pmap.m_vpm.value(), source(h.first, *pmap.m_pm) ),
                      get(pmap.m_vpm.value(), target(h.first, *pmap.m_pm) ) );
  }
};

//property map to access a point from a face_descriptor
template <class PolygonMesh,
          class VertexPointMap = typename boost::property_map<PolygonMesh,vertex_point_t>::type >
struct One_point_from_face_descriptor_map{
  One_point_from_face_descriptor_map()
    : m_pm(nullptr), m_vpm()
  {}

  One_point_from_face_descriptor_map(PolygonMesh const * g)
    : m_pm( const_cast<std::remove_const_t<PolygonMesh>*>(g) )
    , m_vpm( get(vertex_point, *m_pm) )
  {}

  One_point_from_face_descriptor_map(PolygonMesh const * g, VertexPointMap vpm )
    : m_pm( const_cast<std::remove_const_t<PolygonMesh>*>(g) )
    , m_vpm(vpm)
  {}

  std::remove_const_t<PolygonMesh>* m_pm;
  std::optional<VertexPointMap> m_vpm;

  //classical typedefs
  typedef typename boost::graph_traits<PolygonMesh>::face_descriptor key_type;
  typedef typename boost::property_traits< VertexPointMap >::value_type value_type;
  typedef typename boost::property_traits< VertexPointMap >::reference reference;
  typedef boost::readable_property_map_tag category;

  //get function for property map
  inline friend
  reference
  get(const One_point_from_face_descriptor_map<PolygonMesh,VertexPointMap>& m,
      key_type f)
  {
    return get(m.m_vpm.value(), target(halfedge(f, *m.m_pm), *m.m_pm));
  }

  inline friend
  reference
  get(const One_point_from_face_descriptor_map<PolygonMesh,VertexPointMap>& m,
      const std::pair<key_type, const PolygonMesh*>& f)
  {
    return get(m.m_vpm.value(), target(halfedge(f.first, *m.m_pm), *m.m_pm));
  }
};

//property map to access a point from an edge
template < class PolygonMesh,
           class VertexPointMap = typename boost::property_map<PolygonMesh,vertex_point_t>::type >
struct Source_point_from_edge_descriptor_map{
  Source_point_from_edge_descriptor_map()  : m_pm(nullptr), m_vpm()
  {}

  Source_point_from_edge_descriptor_map(PolygonMesh const * g)
    : m_pm( const_cast<std::remove_const_t<PolygonMesh>*>(g) )
    , m_vpm( get(vertex_point, *m_pm) )
  {}

  Source_point_from_edge_descriptor_map(PolygonMesh const * g, VertexPointMap vpm )
    : m_pm( const_cast<std::remove_const_t<PolygonMesh>*>(g) )
    , m_vpm(vpm)
  {}

  //classical typedefs
  typedef typename boost::property_traits< VertexPointMap >::value_type value_type;
  typedef typename boost::property_traits< VertexPointMap >::reference reference;
  typedef typename boost::graph_traits<PolygonMesh>::edge_descriptor key_type;
  typedef boost::readable_property_map_tag category;

  //data
  std::remove_const_t<PolygonMesh>* m_pm;
  std::optional<VertexPointMap> m_vpm;

  //get function for property map
  inline friend
  reference
  get(const Source_point_from_edge_descriptor_map<PolygonMesh,VertexPointMap>& pmap,
      key_type h)
  {
    return get(pmap.m_vpm.value(),  source(h, *pmap.m_pm) );
  }

  inline friend
  reference
  get(const Source_point_from_edge_descriptor_map<PolygonMesh,VertexPointMap>& pmap,
      const std::pair<key_type, const PolygonMesh*>& h)
  {
    return get(pmap.m_vpm.value(),  source(h.first, *pmap.m_pm) );
  }
};

} //namespace CGAL

#endif // CGAL_BOOST_GRAPH_PROPERTY_MAPS_H
