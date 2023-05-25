#define CGAL_TEST_SURFACE_MESH

#include <CGAL/Surface_mesh.h>
#include <CGAL/Simple_cartesian.h>
#include <iostream>
typedef CGAL::Simple_cartesian<double> K;
typedef K::Point_3 Point_3;
typedef CGAL::Surface_mesh<Point_3> Sm;
typedef Sm::Vertex_index Vertex_index;
typedef Sm::Face_index Face_index;
typedef Sm::Edge_index Edge_index;
typedef Sm::Halfedge_index Halfedge_index;
typedef Sm::Vertex_connectivity Vertex_connectivity;
typedef Sm::Halfedge_connectivity Halfedge_connectivity;
typedef Sm::Face_connectivity Face_connectivity;

int main()
{
  Sm m;
  Sm::vertex_index u;

  assert(m.number_of_vertices() == 0);
  assert(m.number_of_removed_vertices() == 0);
  for(int i=0; i < 10; i++){
    u = m.add_vertex(Point_3(0,0,0));
    m.remove_vertex(u);
  }
  assert(m.number_of_vertices() == 0);
  assert(m.number_of_removed_vertices() == 1);


  assert(m.does_recycle_garbage());
  m.set_recycle_garbage(false);
  assert(! m.does_recycle_garbage());

  m.add_vertex(Point_3(0,0,0));
  assert(m.number_of_vertices() == 1);
  assert(m.number_of_removed_vertices() == 1);

  m.set_recycle_garbage(true);
  m.add_vertex(Point_3(0,0,0));
  assert(m.number_of_vertices() == 2);
  assert(m.number_of_removed_vertices() == 0);


  std::cout << m.number_of_vertices() << "  " << m.number_of_removed_vertices() << std::endl;

  // make sure all is OK when clearing the mesh

  auto vconn = m.add_property_map<Vertex_index, Vertex_connectivity>("v:connectivity").first;
  auto hconn = m.add_property_map<Halfedge_index, Halfedge_connectivity>("h:connectivity").first;
  auto fconn = m.add_property_map<Face_index, Face_connectivity>("f:connectivity").first;
  auto vpoint = m.add_property_map<Vertex_index, Point_3>("v:point").first;

  // first call to squat the first available position
  m.add_property_map<Vertex_index, int>("vprop_dummy");
  m.add_property_map<Halfedge_index, int>("hprop_dummy");
  m.add_property_map<Face_index, int>("fprop_dummy");
  m.add_property_map<Edge_index, int>("eprop_dummy");

  auto vprop = m.add_property_map<Vertex_index, int>("vprop").first;
  auto hprop = m.add_property_map<Halfedge_index, int>("hprop").first;
  auto fprop = m.add_property_map<Face_index, int>("fprop").first;
  auto eprop = m.add_property_map<Edge_index, int>("eprop").first;

  {
    m.clear_without_removing_property_maps();

    auto l_vprop = m.add_property_map<Vertex_index, int>("vprop").first;
    auto l_hprop = m.add_property_map<Halfedge_index, int>("hprop").first;
    auto l_fprop = m.add_property_map<Face_index, int>("fprop").first;
    auto l_eprop = m.add_property_map<Edge_index, int>("eprop").first;

    auto l_vconn = m.add_property_map<Vertex_index, Vertex_connectivity>("v:connectivity").first;
    auto l_hconn = m.add_property_map<Halfedge_index, Halfedge_connectivity>("h:connectivity").first;
    auto l_fconn = m.add_property_map<Face_index, Face_connectivity>("f:connectivity").first;
    auto l_vpoint = m.add_property_map<Vertex_index, Point_3>("v:point").first;

    assert( vconn == l_vconn );
    assert( hconn == l_hconn );
    assert( fconn == l_fconn );
    assert( vpoint == l_vpoint );
    assert( vprop == l_vprop );
    assert( hprop == l_hprop );
    assert( fprop == l_fprop );
    assert( eprop == l_eprop );
  }

  {
    m.clear();

    auto l_vconn = m.add_property_map<Vertex_index, Vertex_connectivity>("v:connectivity").first;
    auto l_hconn = m.add_property_map<Halfedge_index, Halfedge_connectivity>("h:connectivity").first;
    auto l_fconn = m.add_property_map<Face_index, Face_connectivity>("f:connectivity").first;
    auto l_vpoint = m.add_property_map<Vertex_index, Point_3>("v:point").first;

    assert( vconn == l_vconn );
    assert( hconn == l_hconn );
    assert( fconn == l_fconn );
    assert( vpoint == l_vpoint );
  }

  return 0;
}
