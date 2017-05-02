/*
 * The purpose of this class is to turn a mess 'simple polygons'
 * into polygons with holes.
 * For example given the 6 polygons bellow
 *    ___________________________________
 *   |    ____________________________   |
 *   |   |  ______________            |  |
 *   |   | |  ___         |   ____    D  |
 *   |   | | A   |   ___  B  |     |  |  |
 *   |   | | |___|  |_C_| |  |     E  |  |
 *   |   | |______________|  |_____|  |  F
 *   |   |____________________________|  |
 *   |___________________________________|
 *
 * The desired result is :
 *  - outter boundary F with hole D         (1)
 *  - outter boundary B with holes A and C  (2)
 *  - outter boundary E with no holes       (3)
 *
 *  1. XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   2. XXXXXXXXXXXXXXXX
 *     XXXXX                            XXXX      XXX   XXXXXXXXXX
 *     XXXXX                            XXXX      XXX   XXXX   XXX
 *     XXXXX                            XXXX      XXXXXXXXXXXXXXXX
 *     XXXXX                            XXXX
 *     XXXXX                            XXXX   3. XXXXXXX
 *     XXXXX                            XXXX      XXXXXXX
 *     XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX      XXXXXXX
 *
 *
 * This is done by creating a tree representing the nesting[1] of the input polygons
 * Each node at an even depth becomes an outter boundary with its children as holes
 * with the previous example :
 *
 *   F           =>     1.  F
 *   `-D                    `-D
 *     +-B       =>     2.  B
 *     | +-A                +-A
 *     | `-C                `-C
 *     `-E       =>     3.  E
 *
 *
 * [1]: I assumed the following:
 *        - a polygon has only one parent
 *        - a polygon A is included in B iff half its points are inside B
 *      I didn't test degenerate cases with multiple/complex overlaps.
 *
 */

#ifndef _POLYGONSSOUP_H_
#define _POLYGONSSOUP_H_


#include<vector>
#include<map>

#include<CGAL/Polygon_with_holes_2.h>

template<class Kernel, class InputIterator, class BackInserter>
class PolygonsSoup {
    private:
        typedef typename CGAL::Polygon_2<Kernel> Polygon;
        typedef typename CGAL::Polygon_with_holes_2<Kernel> PolygonWithHoles;
        typedef typename Polygon::const_iterator PolyPtIterator;
        typedef typename std::vector<std::pair<int,int> > RTree;
        typedef typename std::vector<std::vector<Polygon> > PolygonsGroups;
        typedef typename PolygonsGroups::iterator  PolygonsGroupsIterator;

    public:
        static void getPolygonWithHoles(InputIterator begin, InputIterator end, BackInserter out){
            RTree rtree;
            depthCmp dcmp(&rtree);
            std::vector<Polygon*> polygons;

            int n=0;
            for(InputIterator it = begin; it!= end; ++it, ++n){
                rtree.push_back(std::make_pair(n,-1));
                polygons.push_back(&*it);
                if(it->orientation() == CGAL::CLOCKWISE)
                    it->reverse_orientation();
            }


            bool inclusion[n][n];
            for(int i=0; i<n; ++i)
            for(int j=0; j<n; ++j) if(j!=i){
                inclusion[i][j] = polyInPoly(*polygons.at(i)
                                            ,*polygons.at(j));
            }

            for(int i=0; i<n; ++i){
                std::vector<int> candidateParents
                ,                candidateChildren;
                for(int j=0; j<n; ++j) if(j!=i){
                    if     (inclusion[i][j]) candidateParents.push_back(j);
                    else if(inclusion[j][i]) candidateChildren.push_back(j);
                }

                /* fittest parent is deepest candidate parent */
                if(!candidateParents.empty()){
                    int parent = *max_element(candidateParents.begin()
                                             ,candidateParents.end(), dcmp);
                    rtree.at(i).second = parent;
                }

                /* fittest child is shallowest candidate child */
                if(!candidateChildren.empty()){
                    int child = *min_element(candidateChildren.begin()
                                            ,candidateChildren.end(), dcmp);
                    rtree.at(child).second = i;
                }
            }

            /* max depth = 2, nodes having even depth go back up to surface */
            for(int i=0; i<n; ++i)
                if(candidates_depth(i, &rtree)%2==0)
                    rtree.at(i).second=-1;


            PolygonsGroups groups;
            std::map<int,int> parent2group;
            /* create a group for each root poly (== outter boundary) */
            for(RTree::iterator it=rtree.begin(); it!=rtree.end(); ++it){
                int current = it->first
                ,   parent  = it->second;
                if(parent < 0){
                    std::vector<Polygon> ps;
                    ps.push_back(*polygons.at(current));
                    parent2group[current] = groups.size();
                    groups.push_back(ps);
                }
            }

            /* push each root poly (== hole) in its parent's group */
            for(RTree::iterator it=rtree.begin(); it!=rtree.end(); ++it){
                int current = it->first
                ,   parent  = it->second;
                if(parent > -1){
                    int group = parent2group[parent];
                    groups.at(group).push_back(*polygons.at(current));
                }
            }

            /* now that polygons are grouped, make polygons with holes */
            for(PolygonsGroupsIterator g = groups.begin()
               ;                       g != groups.end(); ++g){
                /* outter boundary must be CCW orientated */
                if(g->front().orientation() != CGAL::COUNTERCLOCKWISE)
                    g->front().reverse_orientation();

                /* holes must be CW orientated */
                for(InputIterator hit =  g->begin()+1
                   ;              hit != g->end(); ++hit)
                    if(hit->orientation() != CGAL::CLOCKWISE)
                        hit->reverse_orientation();

                PolygonWithHoles pwh(g->front(), g->begin()+1, g->end());
                out++ = pwh;
            }
        }

    private:

        static bool polyInPoly(Polygon& a, Polygon& b){
            int in = 0, out = 0;
            int threshold = a.size()/2;
            for(PolyPtIterator pit  = a.vertices_begin()
            ;                  pit != a.vertices_end(); ++pit){
                bool inside = b.bounded_side(*pit) == CGAL::ON_BOUNDED_SIDE;
                if(++(inside? in:out) > threshold) return inside;
            }
            return in > out;
        }


        static int candidates_depth (int i, RTree* rtree) {
            int d = 0;
            while((i=rtree->at(i).second) > -1) ++d;
            return d;
        }


        class depthCmp {
            private : RTree* rt;
            public:
                depthCmp(RTree *rtree): rt(rtree){}
                bool operator() (int ci, int cj) {
                    return candidates_depth(ci, rt) < candidates_depth(cj, rt);
                }
        };
};


#endif
