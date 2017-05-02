"""Generate example figures for the skeleton process"""

import itertools
import ast
import argparse

import polygonskeletons
import asywriter



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
    asy = asywriter.AsyWriter(size=(600,0), flip_y=True)

    # draw the input
    asy.complex_polygon(lines, 'black', 'gray+opacity(0.5)',
                        transform=transform)
    asy.dots(itertools.chain(*lines), 'black',
             transform=transform)

    # redraw the shape of the input before adding the output on top
    asy.complex_polygon(lines, stroke=None, fill='gray+opacity(0.5)')

    # draw the edges of the skeleton
    for u,v in G.edges_iter() :
        asy.line([G.node[u]['xy'],G.node[v]['xy']])

    # draw the nodes as dots + circles of corresponding radius
    for u,d in G.nodes_iter(data=True) :
        asy.dot(d['xy'])
        asy.circle(d['xy'], d['r'], 'black+dotted')

    # compile to pdf
    asy.compile(output_path)
