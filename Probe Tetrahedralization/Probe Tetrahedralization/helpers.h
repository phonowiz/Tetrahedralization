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
#include <thread>
#include "glm/glm.hpp"
#include "glm/gtx/intersect.hpp"



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
    sh9_color sh_color = {};
    glm::vec3 position = {};
    glm::vec3 normal =  {};
};

struct tracing_result
{
    glm::vec3 color;
    glm::vec3 dir;
};

class probe_collector
{
public:
    glm::vec3 ro = glm::vec3(0.0f);
    glm::vec3 rd = glm::vec3(0.0f);
    std::atomic<bool>* done = nullptr;
    std::vector<probe_info>* probes = nullptr;
    
    const asset_vertex_info* vert_info = nullptr;
    
    probe_collector(){};
    probe_collector(glm::vec3 ro, glm::vec3 rd) : ro(ro), rd(rd) {}
    
    bool is_done(){ return *done; }
    
    void operator()()
    {
        (*done) = false;
        const asset_vertex_info& info = *vert_info;
        std::vector<probe_info>& pr = *probes;
       
        //TODO: OPTIMIZE
        for(const triangle& tri : info.triangles)
        {
            glm::vec2 bary_pos = glm::vec2(0.f);
            
            glm::vec3 p0 = info.positions[tri[0]];
            glm::vec3 p1 = info.positions[tri[1]];
            glm::vec3 p2 = info.positions[tri[2]];
            
            float distance = 0.0f;
            
            if(glm::intersectRayTriangle(ro, rd, p0, p1, p2, bary_pos, distance))
            {
                probe_info probe = {};
                glm::vec3 full_bary = glm::vec3(bary_pos, glm::max(1.0f - bary_pos.x - bary_pos.y, 0.0f));
                assert(bary_pos.x + bary_pos.y <= (1.0f + precis));
                //printf("\tinterpolating between\n\t<%f, %f, %f>\n\t<%f, %f, %f>\n\t<%f %f %f>\n", p0.x, p0.y,p0.z, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z);
                probe.position = p0 * full_bary.x + p1 * full_bary.y + p2 * full_bary.z;
                
                glm::vec3 n0 = info.positions[tri[ 0]];
                glm::vec3 n1 = info.positions[tri[1]];
                glm::vec3 n2 = info.positions[tri[2]];
                probe.normal = n0 * full_bary.x + n1 * full_bary.y + n2 * full_bary.z;
                probe.normal = glm::normalize(probe.normal);
                //printf("\thit found at %f, %f, %f at distance: %f\n", probe.position[0],probe.position[1],probe.position[2], distance);
                
                pr.push_back(probe);
            }
        }
        (*done) = true;
    }
};

struct tetrahedra
{
    glm::ivec4          neighbors = {};
    glm::ivec4          probes = {};
    glm::mat3           matrix = glm::mat3(1.0f);
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
    
    in.numberoffacets = 0;
    in.facetlist = nullptr;
    in.facetmarkerlist = nullptr;
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
    
    populate_tetgenio(in, vertices);
}

// based off of https://fizzer.neocities.org/lambertnotangent
glm::vec3 lambert_no_tangent(glm::vec3 normal, glm::vec2 uv)
{
    float theta = 6.283185f * uv.x;
    uv.y = 2.0f * uv.y - 1.0f;
    glm::vec3 sphere_point = glm::vec3(sqrt(1.0f - uv.y * uv.y) * glm::vec2(cos(theta), sin(theta)), uv.y);
    
    assert(!isnan(sphere_point.x) && !isnan(sphere_point.y) && !isnan(sphere_point.z));
    assert(!isnan(normal.x) && !isnan(normal.x) && !isnan(normal.z));
    return normalize(normal + sphere_point);
}

float hash(float x)
{
    return glm::fract(sin(219.151f*x)*9012.15f);
}

glm::vec3 random_ray(glm::vec3 n, glm::vec4 seed)
{
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

struct light_info
{
    glm::vec3 position;
    glm::vec3 color;
} ;


//based off of https://www.shadertoy.com/view/4fyfWR
//glm::vec3 pathtrace(glm::vec3 ro, glm::vec3 rd, bool& collision, float& t)
//{
//    collision = false;
//    t = precis * 2.0f;
//
//    glm::vec3 result = glm::vec3(0.0f);
//    while(std::abs(t) < max_distance)
//    {
//        float h = do_model(ro + rd * t, 0.0f).x;
//        if(( glm::abs(h) < precis))
//        {
//            result =  do_material(ro + rd * t, 0.0f);
//            collision = true;
//            break;
//        }
//        t += precis;
//    }
//    
//    return result;
//}

struct pathtrace_result
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(0.0f);
};



pathtrace_result pathtrace_tri(glm::vec3 ro, glm::vec3 rd, bool& collision, const asset_vertex_info& vert_info)
{
    collision = false;
    
    //TODO: OPTIMIZE THIS
    pathtrace_result result;
    for(const triangle& tri : vert_info.triangles)
    {
        glm::vec3 p0 = vert_info.positions[tri[0]];
        glm::vec3 p1 = vert_info.positions[tri[1]];
        glm::vec3 p2 = vert_info.positions[tri[2]];
        
        glm::vec2 bary_pos = glm::vec2(0.0f);
        float distance = 0.f;
        collision = glm::intersectRayTriangle(ro, rd, p0, p1, p2, bary_pos, distance);
        
        if(collision)
        {
            glm::vec3 n0 = vert_info.normals[tri[0]];
            glm::vec3 n1 = vert_info.normals[tri[1]];
            glm::vec3 n2 = vert_info.normals[tri[2]];
            
            glm::vec3 col0 = vert_info.materials[vert_info.vertex_material[tri[0]]];
            glm::vec3 col1 = vert_info.materials[vert_info.vertex_material[tri[1]]];
            glm::vec3 col2 = vert_info.materials[vert_info.vertex_material[tri[2]]];
            
            glm::vec3 full_bary = glm::vec3(bary_pos, glm::max((1.0f - bary_pos.x - bary_pos.y), 0.0f));
            assert((bary_pos.x + bary_pos.y) <= (1.0f + precis));
            result.position = p0 * full_bary.x + p1 * full_bary.y + p2 * full_bary.z;
            result.normal = n0 * full_bary.x + n1 * full_bary.y + n2 * full_bary.z;
            result.normal = glm::normalize(result.normal);
            result.color = col0 * full_bary.x + col1 * full_bary.y + col2 * full_bary.z;
            
            break;
        }
    }
    
    return result;
}

template<size_t Size =30>
struct pathtracer_info
{
    std::array<tracing_result, Size> results;
};


//static void poor_man_pathtracer(glm::vec3 ro, glm::vec3 rd, tracing_result* results, int index)
//{
//    const int BOUNCE_TOTAL = 2;
//    
//    glm::vec3 color = glm::vec3(0.0f);
//    light_info light =  {};
//    
//    //these must match the shader..
//    light.position = glm::vec3(0.f, 8.2f, .0f);
//    light.color = glm::vec3(.500f, .50f, .500f);
//    
//    glm::vec3 to_light_dir = glm::vec3(0.0f);
//    glm::vec3 surface_normal = glm::vec3(0.0f);
//    
//    float t = 0.0f;
//    for(int i = 0; i < BOUNCE_TOTAL; ++i)
//    {
//        bool collision = false;
//        glm::vec3 surface_color = pathtrace(ro, rd, collision, t);
//        glm::vec3 light_color = glm::vec3(0.0f);
//        glm::vec3 c = ro + rd * t;
//        if(collision)
//        {
//            to_light_dir = light.position - c;
//            to_light_dir = glm::normalize(to_light_dir);
//            surface_normal = calc_normal(c, 0.0f);
//            
//            float to_light_t = precis;
//
//            while( glm::abs(to_light_t) < max_distance)
//            {
//                glm::vec3 test = c + to_light_dir * to_light_t;
//                glm::vec3 l = light.position - test;
//                
//                if( glm::length(l) <  precis)
//                {
//                    light_color = light.color;
//                    break;
//                }
//                
//                float h = do_model(test, 0.0f).x;
//                bool occluded = (glm::abs(h) < precis);
//                if(occluded)
//                    break;
//                to_light_t += precis;
//            }
//        }
//        
//        color += (surface_color * light_color) * glm::max(0.0f, glm::dot(to_light_dir, surface_normal));
//
//        ro = ro + rd * t;
//        
//        glm::vec3 normal = calc_normal(ro, 0.0f);
//        rd = random_ray(normal, glm::vec4(ro, float(i)));
//    }
//    
//    results[index].color = color;
//    results[index].dir = rd;
//}


static void poor_man_pathtracer_tri(glm::vec3 ro, glm::vec3 rd, tracing_result* results, const asset_vertex_info& vert_info, int index)
{
    const int BOUNCE_TOTAL = 2;
    
    glm::vec3 color = glm::vec3(0.0f);
    light_info light =  {};
    glm::vec3 original_rd = rd;
    
    //these must match the shader..
    light.position = glm::vec3(0.f, 1.8f, .0f);
    light.color = glm::vec3(.500f, .50f, .500f);
    //const float max_trace = 20.0f;
    
    glm::vec3 to_light_dir = glm::vec3(0.0f);
    //glm::vec3 surface_normal = glm::vec3(0.0f);
    
    //float t = 0.0f;
    for(int i = 0; i < BOUNCE_TOTAL; ++i)
    {
        bool collision = false;
        //rd = glm::normalize(rd) * max_trace;
        
        pathtrace_result result = pathtrace_tri(ro, rd, collision, vert_info);
        glm::vec3 light_color = glm::vec3(0.0f);
        //glm::vec3 c = result.position;//ro + rd * t;
        if(collision)
        {
            to_light_dir = light.position - result.position;
            //to_light_dir = glm::normalize(to_light_dir);
            //surface_normal = result.normal;//calc_normal(c, 0.0f);
            
            //bfloat to_light_t = precis;

            //while( glm::abs(to_light_t) < max_distance)
            {
                //glm::vec3 test = r + to_light_dir * to_light_t;
                //glm::vec3 l = light.position - test;
                
//                if( glm::length(l) <  precis)
//                {
//                    light_color = light.color;
//                    break;
//                }
                
//                float h = do_model(test, 0.0f).x;
//                bool occluded = (glm::abs(h) < precis);
                
                collision = false;
                pathtrace_tri(result.position, to_light_dir, collision, vert_info);
                if(collision)
                {
                    light_color = light.color;
                }
                    
                //to_light_t += precis;
            }
        }
        
        color += (result.color * light_color) * glm::max(0.0f, glm::dot(to_light_dir, result.normal));

        ro = result.position;//ro + rd * t;
        
        //glm::vec3 normal = calc_normal(ro, 0.0f);
        rd = random_ray(result.normal, glm::vec4(ro, float(i)));
    }
    
    results[index].color = color;
    results[index].dir = original_rd;
}


void generate_probes(tetgenio& in, std::vector<probe_info>& probes, const asset_vertex_info& vert_info )
{
    //NOTE: The following function is very specific to the demo scene
    
    collector_manager manager;
    
    constexpr float total_width = 2.0f;
    constexpr float total_height = 2.0f;
    
    static_assert(total_width == total_height);
    
    
    constexpr float max_tracing_length = 20.0f;
    const float steps =  total_width / 10.0f;

    //X PLANE
    for(float w = -total_width * .5f; w <= total_width * .5f; w += steps)
    {
        for(float h = 0.0f; h <= total_height; h += steps)
        {
            glm::vec3 ro( total_width, h,  w);
            glm::vec3 rd(-max_tracing_length, 0.0f, 0.0f);
            const int index = 0;
            //rd *= max_tracing_length;
            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;

            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;

            manager.collectors[index].vert_info = &vert_info;
            manager.collectors[index]();

            probes.insert(probes.end(), manager.probes[index].begin(), manager.probes[index].end());
            manager.probes[index].clear();
            //count++;
        }

        printf("======done!\n");
    }
    
  
    //Z PLANE
    for(float w = -total_width * .5f; w <= total_width * .5f; w += steps)
    {
        for(float h = 0.0f; h <= total_height ; h += steps)
        {
            glm::vec3 ro(w, h, total_height);
            glm::vec3 rd(0.0f, 0.0f, -max_tracing_length);

            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;

            const int index = 0;

            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;

            manager.collectors[index].vert_info = &vert_info;
            manager.collectors[index]();

            probes.insert(probes.end(), manager.probes[index].begin(), manager.probes[index].end());
            manager.probes[index].clear();
        }
    }
    
    //Y IS UP!!!
    for(float w = -total_width * .5f; w < total_width * .5f; w += steps)
    {
        for(float h = -total_height * .5f; h < total_height * .5f; h += steps)
        {
            glm::vec3 ro(w, total_height, h);
            glm::vec3 rd(0.0f, -max_tracing_length, 0.0f);
            
            //rd *= max_tracing_length;
            
            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;
            
            const int index = 0;
            
            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;

            manager.collectors[index].vert_info = &vert_info;
            manager.collectors[index]();

            probes.insert(probes.end(), manager.probes[index].begin(), manager.probes[index].end());
            manager.probes[index].clear();
        }
    }
    
    populate_tetgenio(in, probes);
}
