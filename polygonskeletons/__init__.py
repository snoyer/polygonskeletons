import os, sys
import subprocess
import itertools
import networkx

CGAL_EXECUTABLE = os.path.join(os.path.dirname(__file__), 'skeleton-cgal')

def skeleton_cgal(polygons) :
    """Compute a straight skeleton for a polygon using an external CGAL process.

    Inputs us a polygon soup as a sequence of a sequences of (x,y)

    The nodes of the resulting graph have the following attributes :
    - `xy`: (x,y) pair for the position of the point
    - `r`: float for the distance to the polygon

    :param lines: polygon soup
    :return: skeleton as a networkx Graph
    """


    # input is one `x0 y0 x1 y1 x2 y2 ...` line for each polygon
    def build_input_lines() :
        for points in polygons :
            yield ' '.join(map(str, itertools.chain.from_iterable(points)))

    # run executable, feeding input through stdin
    process = subprocess.Popen(
        [CGAL_EXECUTABLE],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )

    # run process, raise contents of stderr as error message on failure
    out,err = process.communicate(input='\n'.join(build_input_lines())+'\n')
    if process.returncode != 0 :
        raise OSError(err)

    return read_tgf(out.splitlines())


def read_tgf(lines) :
    G = networkx.Graph()

    lines_iter = iter(lines)

    # read `id label` node lines until read until `#` line
    # labels are `x,y,r`
    for line in itertools.takewhile(lambda l:l.strip()!='#', lines_iter) :
        split = line.strip().split(' ')
        x,y,r = map(float, split[1].split(',',3))
        G.add_node(int(split[0]), xy=(x,y), r=r)

    # read `id1 id2` edge lines until EOF
    for line in lines_iter :
        split = line.strip().split(' ')
        G.add_edge(int(split[0]), int(split[1]))

    return G

def write_tgf(G, out=sys.stdout) :

    for u,d in G. nodes_iter(data=True) :
        (x,y),r = d['xy'], d['r']
        print >>out, u, '%f,%f,%f' % (x,y,r)

    print >>out, '#'

    for u,v in G.edges_iter() :
        print >>out, u, v
