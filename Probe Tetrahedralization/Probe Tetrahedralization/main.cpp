//
//  main.cpp
//  Probe Tetrahedralization
//
//  Created by Rafael Sabino on 12/5/24.
//
#include "tetgen.h"
#include "mathematics.h"
#include "helpers.h"


#include <iostream>

std::atomic_uint32_t segment3d::current_id = 0;

int main(int argc, const char * argv[]) {
    
    tetgenio in, out;
//    tetgenio::facet *f;
//    tetgenio::polygon *p;
//    int i;

    generate_input_tetgen(in);
//    // All indices start from 1.
//    in.firstnumber = 1;
//
//    in.numberofpoints = 8;
//    in.pointlist = new REAL[in.numberofpoints * 3];
//    in.pointlist[0]  = 0.0f;  // vertex 1.
//    in.pointlist[1]  = 0.0f;
//    in.pointlist[2]  = 0.0f;
//    in.pointlist[3]  = 2.0f;  // vertex 2.
//    in.pointlist[4]  = 0.0f;
//    in.pointlist[5]  = 0.0f;
//    in.pointlist[6]  = 2.0f;  // vertex 3.
//    in.pointlist[7]  = 2.0f;
//    in.pointlist[8]  = 0.0f;
//    in.pointlist[9]  = 0.0f;  // vertex 4.
//    in.pointlist[10] = 2.0f;
//    in.pointlist[11] = 0.0f;
//    // Set node 5, 6, 7, 8.
//    for (i = 4; i < 8; i++) {
//      in.pointlist[i * 3]     = in.pointlist[(i - 4) * 3];
//      in.pointlist[i * 3 + 1] = in.pointlist[(i - 4) * 3 + 1];
//      in.pointlist[i * 3 + 2] = 12.0f;
//    }
//
//    in.numberoffacets = 6;
//    in.facetlist = new tetgenio::facet[in.numberoffacets];
//    in.facetmarkerlist = new int[in.numberoffacets];
//
//    // Facet 1. The leftmost facet.
//    f = &in.facetlist[0];
//    f->numberofpolygons = 1;
//    f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
//    f->numberofholes = 0;
//    f->holelist = NULL;
//    p = &f->polygonlist[0];
//    p->numberofvertices = 4;
//    p->vertexlist = new int[p->numberofvertices];
//    p->vertexlist[0] = 1;
//    p->vertexlist[1] = 2;
//    p->vertexlist[2] = 3;
//    p->vertexlist[3] = 4;
//
//    // Facet 2. The rightmost facet.
//    f = &in.facetlist[1];
//    f->numberofpolygons = 1;
//    f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
//    f->numberofholes = 0;
//    f->holelist = NULL;
//    p = &f->polygonlist[0];
//    p->numberofvertices = 4;
//    p->vertexlist = new int[p->numberofvertices];
//    p->vertexlist[0] = 5;
//    p->vertexlist[1] = 6;
//    p->vertexlist[2] = 7;
//    p->vertexlist[3] = 8;
//
//    // Facet 3. The bottom facet.
//    f = &in.facetlist[2];
//    f->numberofpolygons = 1;
//    f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
//    f->numberofholes = 0;
//    f->holelist = NULL;
//    p = &f->polygonlist[0];
//    p->numberofvertices = 4;
//    p->vertexlist = new int[p->numberofvertices];
//    p->vertexlist[0] = 1;
//    p->vertexlist[1] = 5;
//    p->vertexlist[2] = 6;
//    p->vertexlist[3] = 2;
//
//    // Facet 4. The back facet.
//    f = &in.facetlist[3];
//    f->numberofpolygons = 1;
//    f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
//    f->numberofholes = 0;
//    f->holelist = NULL;
//    p = &f->polygonlist[0];
//    p->numberofvertices = 4;
//    p->vertexlist = new int[p->numberofvertices];
//    p->vertexlist[0] = 2;
//    p->vertexlist[1] = 6;
//    p->vertexlist[2] = 7;
//    p->vertexlist[3] = 3;
//
//    // Facet 5. The top facet.
//    f = &in.facetlist[4];
//    f->numberofpolygons = 1;
//    f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
//    f->numberofholes = 0;
//    f->holelist = NULL;
//    p = &f->polygonlist[0];
//    p->numberofvertices = 4;
//    p->vertexlist = new int[p->numberofvertices];
//    p->vertexlist[0] = 3;
//    p->vertexlist[1] = 7;
//    p->vertexlist[2] = 8;
//    p->vertexlist[3] = 4;
//
//    // Facet 6. The front facet.
//    f = &in.facetlist[5];
//    f->numberofpolygons = 1;
//    f->polygonlist = new tetgenio::polygon[f->numberofpolygons];
//    f->numberofholes = 0;
//    f->holelist = NULL;
//    p = &f->polygonlist[0];
//    p->numberofvertices = 4;
//    p->vertexlist = new int[p->numberofvertices];
//    p->vertexlist[0] = 4;
//    p->vertexlist[1] = 8;
//    p->vertexlist[2] = 5;
//    p->vertexlist[3] = 1;
//
//    // Set 'in.facetmarkerlist'
//
//    in.facetmarkerlist[0] = 0;
//    in.facetmarkerlist[1] = 1;
//    in.facetmarkerlist[2] = 2;
//    in.facetmarkerlist[3] = 3;
//    in.facetmarkerlist[4] = 4;
//    in.facetmarkerlist[5] = 5;

    // Output the PLC to files 'barin.node' and 'barin.poly'.
    in.save_nodes("barin");
    in.save_poly("barin");

    // Tetrahedralize the PLC. Switches are chosen to read a PLC (p),
    //   do quality mesh generation (q) with a specified quality bound
    //   (1.414), and apply a maximum volume constraint (a0.1).

    tetrahedralize("nVfc", &in, &out);

    // Output mesh to files 'barout.node', 'barout.ele' and 'barout.face'.
    out.save_nodes("barout");
    out.save_elements("barout");
    out.save_faces("barout");
    out.save_neighbors("barout");

    //return 0;
    return 0;
}
