//
//  helpers.h
//  Probe Tetrahedralization
//
//  Created by Rafael Sabino on 12/6/24.
//

#pragma once

#include "tetgen.h"
#include "mathematics.h"
#include "shapes.h"
#include <vector>
#include <iostream>
#include <chrono>
#include <random>

#include "glm/glm.hpp"



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


struct probe_info
{
    glm::vec3 position = {};
    sh9_color sh_color = {};
};

class probe_collector
{
public:
    glm::vec3 ro = glm::vec3(0.0f);
    glm::vec3 rd = glm::vec3(0.0f);
    std::atomic<bool>* done = nullptr;
    std::vector<probe_info>* probes = nullptr;
    
    float maxd = 10.0f;

    probe_collector(){};
    probe_collector(glm::vec3 ro, glm::vec3 rd) : ro(ro), rd(rd) {}
    
    bool is_done(){ return *done; }
    
    void operator()()
    {
        (*done) = false;
        float t = 0.0f;
        glm::vec3 origin = ro;
        glm::vec3 dir = rd;
        
        std::vector<probe_info>& pr = *probes;
        
        while(std::abs(t) < maxd)
        {
            float h = do_model(ro + rd * t, 0.0f).x;
            
            if(( glm::abs(h) < precis))
            {
                glm::vec3 c = origin + dir * t;
                probe_info info = {};
                info.position = c;
                if(!pr.empty())
                {
                    glm::vec3 v = pr.back().position - c;
                    if(glm::length(v) > (.4f ))
                    {
                        pr.push_back(info);
                    }
                }
                else
                    pr.push_back(info);
            }
            
            t += precis;
        }
        
        (*done) = true;
    }
};

struct tetrahedra
{
    std::array<uint, 4> neighbors = {};
    std::array<uint, 4> probes = {};
};

#define TOTAL_THREADS (20)
struct collector_manager
{

    std::array<std::thread, TOTAL_THREADS>         threads;
    std::array<probe_collector, TOTAL_THREADS>     collectors;
    std::array<std::atomic<bool>, TOTAL_THREADS>   dones;
    std::array<std::vector<probe_info>, TOTAL_THREADS> probes;
    
    collector_manager()
    {
        std::fill(dones.begin(), dones.end(), true);
        
        for( int i = 0; i < TOTAL_THREADS; ++i)
        {
            collectors[i].done = &dones[i];
            collectors[i].probes = &probes[i];
        }
    }
};

float max_distance = 10.0f;



void populate_tetgenio(tetgenio& in,
                       const std::vector<probe_info>& vertices)
{
    in.firstnumber = 0;
    
    in.numberofpoints = (int)vertices.size();
    in.pointlist = new REAL[in.numberofpoints * 3];
    
    for(int i = 0, pnt = 0; i < in.numberofpoints; i++, pnt += 3)
    {
        in.pointlist[pnt + 0] = vertices[i].position.x;
        in.pointlist[pnt + 1] = vertices[i].position.y;
        in.pointlist[pnt + 2] = vertices[i].position.z;
    }
    
    in.numberoffacets = 0;//(int)triangles.size();
    in.facetlist = nullptr;//new tetgenio::facet[in.numberoffacets];
    in.facetmarkerlist = nullptr;//new int[in.numberoffacets];
    
//    int count = 0;ß
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
    std::vector<probe_info> vertices;

    std::unordered_set<triangle3d> triangle_set;

    for(int i = 0; i < max_vertices; ++i)
    {
        probe_info info = {};
        glm::vec3 vec(std::fmod(rand(), 1000.f), std::fmod(rand(), 1000.0f), std::fmod(rand(), 1000.f));
        info.position = vec;
        vertices.push_back(info);
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

// based off of https://fizzer.neocities.org/lambertnotangent
glm::vec3 lambert_no_tangent(glm::vec3 normal, glm::vec2 uv)
{
    float theta = 6.283185f * uv.x;
    uv.y = 2.0f * uv.y - 1.0f;
    glm::vec3 sphere_point = glm::vec3(sqrt(1.0f - uv.y * uv.y) * glm::vec2(cos(theta), sin(theta)), uv.y);
    
    assert(!isnan(sphere_point.x) && !isnan(sphere_point.y) && !isnan(sphere_point.z));
    assert(!isnan(normal.x) && !isnan(normal.y) && !isnan(normal.z));
    return normalize(normal + sphere_point);
}

float hash(float x)
{
    return glm::fract(sin(219.151f*x)*9012.15f);
}

glm::vec3 random_ray(glm::vec3 n, glm::vec4 seed)
{
//    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
//    auto duration = now.time_since_epoch();
    
//    float nanoseconds = (float)rand();//float(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
//    glm::vec2 uv = glm::vec2(hash(51.5f*seed.x + 15.6f*seed.y + 37.1f*seed.z + 13.7f*seed.w + 15.1f*nanoseconds),
//                   hash(19.6f*seed.x + 91.1f*seed.y + 15.1f*seed.z + 21.1f*seed.w + 7.8f*nanoseconds));
//
//    assert(!(std::isnan(uv.x)) && !(std::isnan(uv.x)));
//    glm::vec3 temp =lambert_no_tangent(n, uv);
//    assert(!isnan(temp.x) && !isnan(temp.y) && !isnan(temp.z));
//    return temp;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    while(true)
    {
        glm::vec3 dir = {};
        dir.x = dis(gen) * 2.f - 1.f;
        dir.y = dis(gen) * 2.f - 1.f;
        dir.z = dis(gen) * 2.f - 1.f;
        
        if (glm::length(dir) == 0.0f)
            continue;
        
        dir = glm::normalize(dir);
        if(glm::dot(dir, n) < 0.0f)
            dir *= -1.0f;
        
        return dir;
    }
    
    
}

glm::vec3 do_material( glm::vec3 pos, float /*iTime*/ )
{
    float k = do_model(pos, 0.0f).y;
    
    glm::vec3 c = glm::vec3(0.0);
    
    c = glm::mix(c, glm::vec3(.0f,1.0f,.20f), float(k == 1.0f));
    c = glm::mix(c, glm::vec3(1.0f,0.2f,.1f), float(k == 2.0f));
    c = glm::mix(c, glm::vec3(0.4f,0.3f,1.0f), float(k == 3.0f));
    c = glm::mix(c, glm::vec3(1.f,1.f,1.f), float(k == 4.0f));
    c = glm::mix(c, glm::vec3(0.4f,.0f,0.1f), float(k == 5.0f));
    
    return c;//glm::vec(c,0.0);
}

struct light_info
{
    glm::vec3 position;
    glm::vec3 color;
} ;


//based off of https://www.shadertoy.com/view/4fyfWR
glm::vec3 pathtrace(glm::vec3 ro, glm::vec3 rd, bool& collision, float& t)
{
    //glm::vec3 atten = glm::vec3(1.0f, 1.0f, 1.0f);
    collision = false;
    t = precis * 2.0f;

    glm::vec3 result = glm::vec3(0.0f);
    while(std::abs(t) < max_distance)
    {
        float h = do_model(ro + rd * t, 0.0f).x;
        if(( glm::abs(h) < precis))
        {
            //t = 0;
            //ro = ro + rd * t;
            //rd = light.position - ro;
            //rd = glm::normalize(rd);
            
            result =  do_material(ro + rd * t, 0.0f);
            collision = true;
            break;
            
//            bool can_see_light = false;
//            while( glm::abs(t) < max_distance)
//            {
//                glm::vec3 c = ro + rd * t;
//                glm::vec3 l = light.position - c;
//
//                color_gathered *= do_material(c, 0.0f);
//                if( glm::length(l) <  precis * 2.0f)
//                {
//                    result = atte
//                    can_see_light = false;
//                    break;
//                }
//            }
//        }
//        else
        }
        t += precis;
    }
    
    return result;
}

glm::vec3 poor_man_raytracer(glm::vec3 ro, glm::vec3 rd)
{
    const int BOUNCE_TOTAL = 2;
    
    glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);
    light_info light =  {};
    
    //these must match the shader..
    light.position = glm::vec3(0.f, 8.2f, .0f);
    light.color = glm::vec3(20.0, 20.20, 20.050) * 5.0f;
    
    glm::vec3 to_light_dir = glm::vec3(0.0f);
    glm::vec3 surface_normal = glm::vec3(0.0f);
    
    float t = 0.0f;
    for(int i = 0; i < BOUNCE_TOTAL; ++i)
    {
        bool collision = false;
        glm::vec3 surface_color = pathtrace(ro, rd, collision, t);
        glm::vec3 light_color = {};
        glm::vec3 c = ro + rd * t;
        if(collision)
        {
            to_light_dir = light.position - c;
            to_light_dir = glm::normalize(to_light_dir);
            surface_normal = calc_normal(c, 0.0f);
            
            float to_light_t = precis * 2.0f;

            while( glm::abs(to_light_t) < max_distance)
            {
                glm::vec3 test = c + to_light_dir * to_light_t;
                glm::vec3 l = light.position - test;
                
                if( glm::length(l) <  precis * 2.0f)
                {
                    light_color = light.color;
                    break;
                }
                
                float h = do_model(test, 0.0f).x;
                bool occluded = (glm::abs(h) < precis);
                if(occluded)
                    break;
                to_light_t += precis;
            }
        }
        
        color += (surface_color * light_color) * glm::max(0.0f, glm::dot(to_light_dir, surface_normal));

        ro = ro + rd * t;
        
        glm::vec3 normal = calc_normal(ro, 0.0f);
        rd = random_ray(normal, glm::vec4(ro, float(i)));
    }
    
    return color;
}
