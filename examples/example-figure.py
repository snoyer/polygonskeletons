"""Generate example figures for the skeleton process"""

from collections import namedtuple
import itertools
import ast
import argparse

import polygonskeletons
import asywriter


Circle = namedtuple('Circle', 'c r')
@asywriter.drawable_converter(Circle)
def asy_Circle(c) :
    return 'path', 'circle((%f,%f),%f)' % (c.c[0], c.c[1], c.r)

if __name__ == '__main__' :
    parser = argparse.ArgumentParser()
    parser.add_argument('input_path')
    parser.add_argument('output_path')
    args = parser.parse_args()

    input_path = args.input_path
    output_path = args.output_path

    # read lines from input file
    lines = ast.literal_eval(''.join(open(input_path)))

    # compute skeleton
    G = polygonskeletons.skeleton_cgal(lines)


    # compute extent of input so we can make a transform
    # to draw input and output side by side
    xs,ys = zip(*itertools.chain(*lines))
    w = max(xs) - min(xs)
    h = max(ys) - min(ys)
    transform = 'shift(%f,%f)' % ((-w*1.1, 0) if h > w else (0, h*1.1))

    # Asymptote writer
    asy = asywriter.AsyWriter(size=(800,0), flip_y=True)

    # draw the input
    asy.draw(lines, 'black', 'gray+opacity(0.5)', transform=transform)
    asy.dot(itertools.chain(*lines), 'black', transform=transform)

    # redraw the shape of the input before adding the output on top
    asy.draw(lines, stroke=None, fill='gray+opacity(0.5)')

    # draw the edges of the skeleton
    for u,v in G.edges_iter() :
        if G.node[u]['d'] or G.node[v]['d'] :
            pen = 'black' if G.node[u]['d'] and G.node[v]['d'] else 'grey'
            asy.draw([G.node[u]['xy'],G.node[v]['xy']], pen)

    # draw the nodes as dots + circles of corresponding radius
    for u,d in G.nodes_iter(data=True) :
        if d['d'] :
            asy.dot(d['xy'])
            asy.draw(Circle(d['xy'], d['d']), 'black+dotted')


    # compile to pdf
    asy.compile(output_path)
