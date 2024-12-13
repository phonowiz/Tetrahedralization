//
//  helpers.h
//  Probe Tetrahedralization
//
//  Created by Rafael Sabino on 12/6/24.
//

#pragma once

#include "tetgen.h"
#include "mathematics.h"
#include <vector>
#include <iostream>



//void find_triangles(std::vector<vector3d>& verteces,
//                   std::vector<uint32_t>& path, uint32_t origin, uint32_t vertex_index, std::unordered_set<triangle3d>& triangle_map)
//{
//    path.push_back(vertex_index);
//
//    if(path.size() == 3)
//    {
//        for(segment3d segment : verteces[vertex_index].segments)
//        {
//            assert(segment.p1 != segment.p2);
//            assert(segment.p1 == vertex_index || segment.p2 == vertex_index);
//
//            if((segment.p1 == origin || segment.p2 == origin))
//            {
//                triangle3d t(path[0], path[1], path[2]);
//                if(t.is_valid() && !triangle_map.contains(t))
//                    triangle_map.insert(t);
//            }
//        }
//    }
//    else
//    {
//
//        for(segment3d segment : verteces[vertex_index].segments)
//        {
//            assert(segment.p1 == vertex_index || segment.p2 == vertex_index);
//
//            uint32_t next_vertex = segment.p1 == vertex_index ? segment.p2 : segment.p1;
//            if(path.size() == 1)
//                origin = vertex_index;
//
//            find_triangles(verteces, path, origin, next_vertex, triangle_map);
//        }
//    }
//
//    path.pop_back();
//}

//bool triangle_intersect_no_edge_or_vertex(const triangle3d& tri1, const triangle3d& tri2, std::vector<vector3d>& vertices)
//{
//    vector3d& a1 = vertices[tri1.pt1];
//    vector3d& a2 = vertices[tri1.pt2];
//    vector3d& a3 = vertices[tri1.pt3];
//
//    vector3d& b1 = vertices[tri2.pt1];
//    vector3d& b2 = vertices[tri2.pt2];
//    vector3d& b3 = vertices[tri2.pt3];
//
//    bool coplanar = false;
//    std::vector<REAL> intersect_pt1(3);
//    std::vector<REAL> intersect_pt2(3);
//
//    bool intersect = threeyd::moeller::TriangleIntersectsFloats::triangle(a1.coords, a2.coords, a3.coords,
//                                       b1.coords, b2.coords, b3.coords, intersect_pt1, intersect_pt2, coplanar);
//
//    vector3d vpt1 (intersect_pt1);
//    vector3d vpt2 (intersect_pt2);
//
//    if((vpt1 - vpt2).epsion_equal())
//    {
//        std::array<vector3d, 6> vertices = {a1, a2, a3, b1, b2, b3};
//        for(vector3d& v : vertices)
//        {
//            const vector3d x = v - vpt1;
//            if(x.epsion_equal())
//            {
//                intersect = false;
//                break;
//            }
//        }
//    }
//    else
//    {
//        std::array<vector3d, 2> potential_edge = {vpt1 - vpt2, vpt2 - vpt1};
//        std::array<vector3d, 6> edges = { a1 - a2, a2 - a3, a3 - a1, b1 - b2, b2 - b3, b3 - b1};
//
//        auto iter = edges.begin();
//        while(iter != edges.end() && intersect)
//        {
//            for(const vector3d& p : potential_edge)
//            {
//                vector3d e = *iter;
//                vector3d x = e - p;
//                if(x.epsion_equal())
//                {
//                    intersect = false;
//                    break;
//                }
//                else
//                {
//                    x = p - e;
//                    if(x.epsion_equal())
//                    {
//                        intersect = false;
//                        break;
//                    }
//                }
//            }
//            ++iter;
//        }
//    }
//
//    return intersect;
//}


void populate_tetgenio(tetgenio& in,
                       const std::vector<glm::vec3>& vertices)
{
    in.firstnumber = 0;
    
    in.numberofpoints = (int)vertices.size();
    in.pointlist = new REAL[in.numberofpoints * 3];
    
    for(int i = 0, pnt = 0; i < in.numberofpoints; i++, pnt += 3)
    {
        in.pointlist[pnt + 0] = vertices[i].x;
        in.pointlist[pnt + 1] = vertices[i].y;
        in.pointlist[pnt + 2] = vertices[i].z;
    }
    
    in.numberoffacets = 0;//(int)triangles.size();
    in.facetlist = nullptr;//new tetgenio::facet[in.numberoffacets];
    in.facetmarkerlist = nullptr;//new int[in.numberoffacets];
    
//    int count = 0;
//    for(const triangle3d& tri : triangles)
//    {
//        tetgenio::facet *f = &in.facetlist[count];
//        f->numberofpolygons = 1;
//        f->polygonlist = new tetgenio::polygon();
//        f->numberofholes = 0;
//        f->holelist = nullptr;
//        
//        tetgenio::polygon *p = f->polygonlist;
//        p->numberofvertices = 3;
//        p->vertexlist = new int[p->numberofvertices];
//        p->vertexlist[0] = tri.pt1;
//        p->vertexlist[1] = tri.pt2;
//        p->vertexlist[2] = tri.pt3;
//        
//        in.facetmarkerlist[count] = count;
//        count++;
//    }
    
}


void generate_input_tetgen(tetgenio& in)
{
    static constexpr REAL max_vertices = REAL(50.0);

    in.firstnumber = 0;
    in.numberofpoints = max_vertices;
    std::vector<glm::vec3> vertices;

    std::unordered_set<triangle3d> triangle_set;

    for(int i = 0; i < max_vertices; ++i)
    {
        glm::vec3 vec(std::fmod(rand(), 1000.f), std::fmod(rand(), 1000.0f), std::fmod(rand(), 1000.f));
        vertices.push_back(vec);
    }
//
//    for(int i = 0; i < max_vertices; ++i)
//    {
//        for(int j = 0; j < max_vertices; ++j)
//        {
//            if(i == j)
//                continue;
//            vertices[i].segments.push_back(segment3d(i, j));
//        }
//    }
//
//    std::vector<uint32_t> path;
//    uint32_t origin = 0;
//    std::vector<triangle3d> triangles;
//    uint32_t vertex_index = 0;
//    find_triangles(vertices, path, origin, vertex_index, triangle_set);
//
//    std::unordered_set<triangle3d> final_triangles;
//    std::unordered_set<triangle3d> removed_triangles;
//
//    for(const triangle3d& tri : triangle_set)
//    {
//        bool intersect = false;
//        for(const triangle3d& tri2 : triangle_set)
//        {
//            if(tri == tri2)
//                continue;
//
//            intersect = triangle_intersect_no_edge_or_vertex(tri, tri2, vertices);
//            if(intersect)
//                break;
//        }
//
//        if(intersect)
//            removed_triangles.insert(tri);
//        else
//            final_triangles.insert(tri);
//    }
//
//    auto removed_iter = removed_triangles.begin();
//    while(removed_iter != removed_triangles.end())
//    {
//        bool intersect = false;
//        const triangle3d& removed = *removed_iter;
//
//        for(const triangle3d& final : final_triangles)
//        {
//            if(removed != final)
//            {
//                intersect = triangle_intersect_no_edge_or_vertex(final, removed, vertices);
//                if(intersect)
//                    break;
//            }
//        }
//
//        if(!intersect)
//            final_triangles.insert(removed);
//
//        removed_iter++;
//    }
    
    populate_tetgenio(in, vertices);
    
}
