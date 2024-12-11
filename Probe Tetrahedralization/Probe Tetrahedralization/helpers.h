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



void find_triangles(std::vector<vector3d>& verteces,
                   std::vector<uint32_t>& path, uint32_t origin, uint32_t vertex_index, std::unordered_set<triangle3d>& triangle_map)
{
    path.push_back(vertex_index);
    
    if(path.size() == 3)
    {
        //segment3d segment = verteces[vertex_index].segments.back();
        for(segment3d segment : verteces[vertex_index].segments)
        {
            assert(segment.p1 != segment.p2);
            assert(segment.p1 == vertex_index || segment.p2 == vertex_index);
            
            if((segment.p1 == origin || segment.p2 == origin))
            {
                triangle3d t(path[0], path[1], path[2]);
                if(t.is_valid() && !triangle_map.contains(t))
                    triangle_map.insert(t);
            }
        }
    }
    else
    {
        
        for(segment3d segment : verteces[vertex_index].segments)
        {
            assert(segment.p1 == vertex_index || segment.p2 == vertex_index);
            
            uint32_t next_vertex = segment.p1 == vertex_index ? segment.p2 : segment.p1;
            if(path.size() == 1)
                origin = vertex_index;
            
            find_triangles(verteces, path, origin, next_vertex, triangle_map);
        }
    }

    path.pop_back();
}

bool triangle_intersect_no_edge_or_vertex(const triangle3d& tri1, const triangle3d& tri2, std::vector<vector3d>& vertices)
{
    vector3d& a1 = vertices[tri1.pt1];
    vector3d& a2 = vertices[tri1.pt2];
    vector3d& a3 = vertices[tri1.pt3];
    
    vector3d& b1 = vertices[tri2.pt1];
    vector3d& b2 = vertices[tri2.pt2];
    vector3d& b3 = vertices[tri2.pt3];
    
    bool coplanar = false;
    std::vector<REAL> intersect_pt1(3);
    std::vector<REAL> intersect_pt2(3);
    
    bool intersect = threeyd::moeller::TriangleIntersectsFloats::triangle(a1.coords, a2.coords, a3.coords,
                                       b1.coords, b2.coords, b3.coords, intersect_pt1, intersect_pt2, coplanar);
    
    vector3d vpt1 (intersect_pt1);
    vector3d vpt2 (intersect_pt2);
    
    if((vpt1 - vpt2).epsion_equal())
    {
        std::array<vector3d, 6> vertices = {a1, a2, a3, b1, b2, b3};
        for(vector3d& v : vertices)
        {
            vector3d x = v - vpt1;
            if(x.epsion_equal())
            {
                intersect = false;
                break;
            }
        }
    }
    else
    {
        std::array<vector3d, 2> potential_edge = {vpt1 - vpt2, vpt2 - vpt1};
        std::array<vector3d, 6> edges = { a1 - a2, a2 - a3, a3 - a1, b1 - b2, b2 - b3, b3 - b1};
        
        auto iter = edges.begin();
        while(iter != edges.end() && intersect)
        {
            for(const vector3d& p : potential_edge)
            {
                vector3d x = *iter - p;
                if(x.epsion_equal())
                {
                    intersect = false;
                    break;
                }
            }
            ++iter;
        }
    }
    
    return intersect;
}
void generate_input_tetgen(tetgenio& in)
{
    static constexpr REAL num_triangles = 2.0f;
    static constexpr REAL max_vertices = num_triangles * 3.0f;
    //static constexpr REAL num_randoms = max_vertices * 3.0f;
    
    in.firstnumber = 0;
    in.numberofpoints = max_vertices;
    std::vector<vector3d> vertices;
 
    std::unordered_set<triangle3d> triangle_set;
    
    for(int i = 0; i < max_vertices; ++i)
    {
        vector3d vec(std::fmod(rand(), 20.f), std::fmod(rand(), 20.0f), std::fmod(rand(), 20.f));
        vertices.push_back(vec);
    }

    //TODO: generate segment to every other vertex, there will be duplicates, how to hash a segment??
    for(int i = 0; i < max_vertices; ++i)
    {
        for(int j = 0; j < max_vertices; ++j)
        {
            if(i == j)
                continue;
            
            vertices[i].segments.push_back(segment3d(i, j));
            
        }
    }
    
    std::vector<uint32_t> path;
    uint32_t origin = 0;
    std::vector<triangle3d> triangles;
    uint32_t vertex_index = 0;
    find_triangles(vertices, path, origin, vertex_index, triangle_set);
    
    std::unordered_set<triangle3d> final_triangles;
    std::unordered_set<triangle3d> removed_triangles;
    
    for(const triangle3d& tri : triangle_set)
    {
        bool intersect = false;
        for(const triangle3d& tri2 : triangle_set)
        {
            if(tri == tri2)
                continue;
            
            intersect = triangle_intersect_no_edge_or_vertex(tri, tri2, vertices);

            if(intersect)
                break;
        }
        
        if(intersect)
            removed_triangles.insert(tri);
        else
            final_triangles.insert(tri);
    }
    
    //for(const triangle3d& removed : removed_triangles)
    auto removed_iter = removed_triangles.begin();
    while(removed_iter != removed_triangles.end())
    {
        bool intersect = false;
        const triangle3d& removed = *removed_iter;
        
        for(const triangle3d& final : final_triangles)
        {
            if(*removed_iter != final)
            {
                vector3d& a1 = vertices[removed.pt1];
                vector3d& a2 = vertices[removed.pt2];
                vector3d& a3 = vertices[removed.pt3];
                
                vector3d& b1 = vertices[final.pt1];
                vector3d& b2 = vertices[final.pt2];
                vector3d& b3 = vertices[final.pt3];
                
                intersect = threeyd::moeller::TriangleIntersectsFloats::triangle(a1.coords, a2.coords, a3.coords,
                                                   b1.coords, b2.coords, b3.coords);
                if(intersect)
                    break;
            }
        }
        
        if(!intersect)
            final_triangles.insert(removed);
        
        removed_iter++;
    }
    
}
