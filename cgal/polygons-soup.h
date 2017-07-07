/*
 * The purpose of this class is to turn a mess 'simple polygons'
 * into polygons with holes.
 * For example given the 6 polygons bellow
 *   .->---------------------------------.
 *   |   .-<--------------------------.  |
 *   |   | .->------------.           |  |
 *   |   | | .-<-.        |  .->---.  D  |
 *   |   | | A   |  .-<-. B  |     |  |  |
 *   |   | | `---'  C---' |  |     E  |  |
 *   |   | `--------------'  `-----'  |  F
 *   |   `----------------------------'  |
 *   `-----------------------------------'
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
 * This is done by creating a tree representing the nesting of the input polygons
 * Each node at an even depth becomes an outter boundary with its children as holes
 * with the previous example :
 *
 *  -F           =>     1. -F
 *   `-D                    `-D
 *     +-B       =>     2. -B
 *     | +-A                +-A
 *     | `-C                `-C
 *     `-E       =>     3. -E
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

    public:
        static void getPolygonWithHoles(InputIterator begin, InputIterator end, BackInserter out){
            RTree rtree;
            std::vector<Polygon*> polygons;

            int n=0;
            for(InputIterator it = begin; it!= end; ++it, ++n){
                rtree.push_back(std::make_pair(n,-1));
                polygons.push_back(&*it);
            }

            std::vector<std::vector<unsigned>> includedIn;
            for(int i=0; i<n; ++i){
                std::vector<unsigned> indices;
                for(unsigned j=0; j<n; ++j)
                    if(i!=j && polyInPoly(*polygons.at(i)
                                         ,*polygons.at(j)))
                        indices.push_back(j);
                includedIn.push_back(indices);
            }

            depthCmp dcmp(&includedIn);
            for(unsigned i=0; i<n; ++i){
                if(!includedIn[i].empty()){
                    unsigned parent = *max_element(includedIn[i].begin()
                                                  ,includedIn[i].end(), dcmp);
                    rtree.at(i).second = parent;
                }
            }

            /* max depth = 2, nodes having even depth go back up to surface */
            for(int i=0; i<n; ++i)
                if(candidates_depth(i, &rtree)%2==0)
                    rtree.at(i).second = -1;

            /* holes cannot be the same orientation as their parent */
            for(auto& pair : rtree)
                if(pair.second > -1){
                    const Polygon* a = polygons.at(pair.first);
                    const Polygon* b = polygons.at(pair.second);
                    if(a->orientation() == b->orientation())
                        pair.second = -1;
                }


            PolygonsGroups groups;
            std::map<int,int> parent2group;
            /* create a group for each root poly (== outter boundary) */
            for(const auto& pair : rtree){
                int current = pair.first
                ,   parent  = pair.second;
                if(parent < 0){
                    std::vector<Polygon> ps;
                    ps.push_back(*polygons.at(current));
                    parent2group[current] = groups.size();
                    groups.push_back(ps);
                }
            }

            /* push each root poly (== hole) in its parent's group */
            for(const auto& pair : rtree){
                int current = pair.first
                ,   parent  = pair.second;
                if(parent > -1){
                    int group = parent2group[parent];
                    groups.at(group).push_back(*polygons.at(current));
                }
            }

            /* now that polygons are grouped, make polygons with holes */
            for(auto& g : groups){
                /* outter boundary must be CCW orientated */
                if(g.front().orientation() != CGAL::COUNTERCLOCKWISE)
                    g.front().reverse_orientation();

                /* holes must be CW orientated */
                for(InputIterator hit = g.begin()+1; hit != g.end(); ++hit)
                    if(hit->orientation() != CGAL::CLOCKWISE)
                        hit->reverse_orientation();

                PolygonWithHoles pwh(g.front(), g.begin()+1, g.end());
                out++ = pwh;
            }
        }

    private:

        static bool polyInPoly(const Polygon& a, const Polygon& b){
            for(PolyPtIterator pit  = a.vertices_begin()
            ;                  pit != a.vertices_end(); ++pit){
                if(b.bounded_side(*pit) == CGAL::ON_UNBOUNDED_SIDE)
                    return false;
            }
            return true;
        }


        static int candidates_depth(int i, RTree* rtree) {
            int d = 0;
            while((i=rtree->at(i).second) > -1) ++d;
            return d;
        }


        class depthCmp {
            private : std::vector<std::vector<unsigned>>* _vs;
            public:
                depthCmp(std::vector<std::vector<unsigned>> *vs): _vs(vs){}
                bool operator() (unsigned ci, unsigned cj) {
                    return _vs->at(ci).size() < _vs->at(cj).size();
                }
        };
};


#endif
