#include<vector>
#include<iterator>
#include<iostream>
#include<iomanip>
#include<string>
#include<fstream>
#include<sstream>

#include<boost/shared_ptr.hpp>

#include <CGAL/AABB_tree.h> // must be inserted before kernel
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_segment_primitive.h>

#include<CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include<CGAL/Exact_predicates_exact_constructions_kernel.h>

#include<CGAL/Polygon_with_holes_2.h>
#include<CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>


#include "polygons-soup.h"


typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
// typedef CGAL::Exact_predicates_exact_constructions_kernel K;
//TODO refactor to be able to choose at runtime (or use flags and compile both version)

typedef K::Point_2                    Point;
typedef CGAL::Polygon_2<K>            Polygon;
typedef CGAL::Polygon_with_holes_2<K> PolygonWithHoles;

typedef CGAL::Straight_skeleton_2<K>  StraightSkeleton;
typedef CGAL::Straight_skeleton_2<K>::Halfedge_const_handle   Halfedge_const_handle ;
typedef CGAL::Straight_skeleton_2<K>::Vertex_const_handle     Vertex_const_handle ;

typedef typename K::Segment_3 AabbSegment;
typedef typename K::Point_3 AabbPoint;
typedef typename std::list<AabbSegment>::iterator SegmentIterator;
typedef typename CGAL::AABB_segment_primitive<K, SegmentIterator> Primitive;
typedef typename CGAL::AABB_traits<K, Primitive> Traits;
typedef typename CGAL::AABB_tree<Traits> AabbTree;

struct lt_Vertex_handle {
    bool operator()(const Vertex_const_handle& a, const Vertex_const_handle& b) const{
        return &*a < &*b;
    }
};

typedef boost::shared_ptr<StraightSkeleton> StraightSkeleton_ptr;


int main( int argc, char* argv[]) {

    /* read each "x0 y0 x1 y1 x2 y2 ..." line from stdin as a polygon */
    std::vector<Polygon> polygons;
    {
        const double sq_epsilon = .00001 * .00001;
        std::string line;
        double x,y;
        while(std::cin) {
            getline(std::cin,line);
            if(line.length()>2) {
                std::istringstream iss(line);
                std::vector<Point> pts;
                do{
                    iss >> x >> y;
                    Point pt(x,y);
                    if(pts.empty() || CGAL::squared_distance(pt, pts.back()) >= sq_epsilon)
                        pts.push_back(pt);
                }while(iss);

                /* make sure we don't duplicate the first/last point when closing
                   otherwise CGAL gets confused about orientation and we can't have that */
                while(!pts.empty() && CGAL::squared_distance(pts.front(), pts.back()) < sq_epsilon)
                    pts.pop_back();

                if(pts.size() > 2) {
                    Polygon p(pts.begin(), pts.end());
                    polygons.push_back(p);
                }
            }
        }
    }

    /* sort out polygon soup */
    std::vector<PolygonWithHoles> pwhs;
    PolygonsSoup<K ,std::vector<Polygon>::iterator
                   ,std::back_insert_iterator<std::vector<PolygonWithHoles> >
    >::getPolygonWithHoles(polygons.begin(), polygons.end(), std::back_inserter(pwhs));


    /* insert all segments from each polygon into an AABB tree */
    std::list<AabbSegment> segments;
    for(Polygon p : polygons) {
        std::vector<AabbPoint> pointsAabb;
        for(auto vit = p.vertices_begin(); vit != p.vertices_end(); ++vit)
            pointsAabb.push_back(AabbPoint(vit->x(),vit->y(), 0));
        const size_t np = pointsAabb.size();
        for(int pi=0; pi<np; ++pi)
            segments.push_back(AabbSegment(
                pointsAabb.at(pi), pointsAabb.at((pi+1)%np)
            ));
    }
    AabbTree tree(segments.begin(),segments.end());
    tree.accelerate_distance_queries();


    /* compute straight skeletons */
    std::vector<StraightSkeleton_ptr> sss;
    for(const auto& pwh:pwhs)
        sss.push_back(CGAL::create_interior_straight_skeleton_2(pwh));


    /* keep track of vertex handle -> node id mapping*/
    std::map<Vertex_const_handle, int, lt_Vertex_handle> vIndexes;
    size_t index = 0;

    /* print tgf `id label` node lines with `x,y,dist` as label */
    for(auto& ss : sss) {
        for(auto vit = ss->vertices_begin(); vit != ss->vertices_end(); ++vit){
            const Vertex_const_handle v = vit;
            if(v->is_skeleton()) {
                const Point p = v->point();
                const AabbPoint aabbQ(p.x(),p.y(),0);
                const AabbPoint aabbR = tree.closest_point(aabbQ);
                const K::FT d = CGAL::squared_distance(aabbQ,aabbR);

                std::cout << index << " "
                          << CGAL::to_double(p.x()) << ","
                          << CGAL::to_double(p.y()) << ","
                          << sqrt(CGAL::to_double(d)) << std::endl;

                vIndexes.insert(std::make_pair(v, index));
                ++index;
            }
        }
    }

    /* print tgf separator line */
    std::cout<< "#" << std::endl;

    /* print tgf `id1 id2` edge lines */
    for(auto& ss : sss) {
        for(auto hit = ss->halfedges_begin(); hit != ss->halfedges_end(); ++hit){
            const Halfedge_const_handle h = hit;
            const Vertex_const_handle& v1 = h->vertex();
            const Vertex_const_handle& v2 = h->opposite()->vertex();
            if(v1->is_skeleton() && v2->is_skeleton())
                if(&*v1 < &*v2)
                    std::cout << vIndexes[v1] << " "
                              << vIndexes[v2] << std::endl ;
        }
    }

    return 0;
}
