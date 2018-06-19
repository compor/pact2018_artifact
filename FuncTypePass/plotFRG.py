import sys
import pygraphviz as pgv

f = open(sys.argv[1], 'r')
f1 = f.readlines()

G = pgv.AGraph(directed=True)


edge_labels = dict()

for e in f1:
    ee = e.split()
    #G.add_edge(ee[0], ee[1], label=ee[2] + ", " + (str(float(ee[3])) if ee[3] != 'NULL' else 'NULL')  + (', pass origin' if ee[4].strip() =='1' else ''))
    G.add_edge(ee[0], ee[1], label=ee[2])

G.layout(prog='dot')

G.draw(sys.argv[1]+'.png', format='png')

