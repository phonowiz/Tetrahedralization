//
//  shapes.h
//  Probe Tetrahedralization
//
//  Created by Rafael Sabino on 12/11/24.
//

#pragma once

#include "glm/glm.hpp"

#include <array>
#include "mathematics.h"
#include <numbers>
#include <vector>

/*
struct material
{
    vector3d albedo;
    vector3d emission;
    float specular;
};
    
struct sphere
{
    vector3d p;
    float r;
    material mat;
    
    void generate_probes_positions(std::vector<vector3d>& positions);
};

struct plane
{
    vector3d p;
    vector3d n;
    material mat;
    
    void generate_probes_positions(std::vector<vector3d>& positions);
};
    
struct disk
{
    vector3d p;
    vector3d n;
    float r;
    material mat;
    
    void generate_probes_positions(std::vector<vector3d>& positions);
};


void sphere::generate_probes_positions(std::vector<vector3d>& positions)
{
    constexpr float to_360 = 2.0f * std::numbers::pi;
    constexpr float to_180 = std::numbers::pi;
    
    constexpr float steps = 30.0f;
    constexpr float to_360_steps = to_360 / steps;
    constexpr float to_180_steps = to_180 / steps;
    
    for(float theta = 0.0f; theta < to_360; theta += to_360_steps )
    {
        for(float phi = 0.0f; phi < to_180; phi += to_180_steps)
        {
            float x = r * std::cosf(theta) * std::sinf(phi);
            float y = r * std::sinf(theta) * std::sinf(phi);
            float z = r * std::cosf(phi);
            
            vector3d surface(x, y, z);
            vector3d pos = surface + p;
            positions.push_back(pos);
            
        }
    }
}

void plane::generate_probes_positions(std::vector3d<vector)
{

}

*/


glm::vec2 min2(glm::vec2 a, glm::vec2 b)
{
    return (a.x <= b.x)?a:b;
}

glm::vec2 max2(glm::vec2 a, glm::vec2 b)
{
    return (a.x > b.x)?a:b;
}

float sdSphere( glm::vec3 p, float s )
{
    return glm::length(p)-s;
}

float sdCylinder( glm::vec3 p, float s )
{
    glm::vec2 d = glm::vec2(p.x, p.z);
    return glm::length(d)-s;
}

float sdTorus( glm::vec3 p, glm::vec2 t )
{
    glm::vec2 xz(p.x, p.z);
    glm::vec2 q = glm::vec2(glm::length(xz)-t.x,p.y);
    return glm::length(q)-t.y;
}

float sdBox( glm::vec3 p, glm::vec3 b )
{
    glm::vec3 d = glm::abs(p) - b;
    
    return glm::min(glm::max(d.x,glm::max(d.y,d.z)),0.0f) +
         glm::length(glm::max(d,0.0f));
}

glm::vec2 plane( glm::vec3 p) {
    return glm::vec2(p.y+1.0,4.0);
}

glm::vec2 do_model( glm::vec3 p, float /*itime*/ ) {
    
    glm::vec2 d = plane(p);

    glm::vec2 q = glm::vec2(sdSphere(p - glm::vec3(0.0f,0.0f,-0.8f), 1.0f),1.0f);
    q = max2(q, glm::vec2(-sdCylinder(p - glm::vec3(0.0f,0.0f,-0.8f), 0.5f),2.0f));
    d = min2(d, q);

    d = min2(d, glm::vec2(sdBox(p - glm::vec3(0.0f,0.0f,2.2f), glm::vec3(2.0f,4.0f,0.3f)),2.0f));
    d = min2(d, glm::vec2(sdBox(p - glm::vec3(0.0f,0.0f,-2.2f), glm::vec3(2.0f,4.0f,0.3f)),3.0f));
    d = min2(d, glm::vec2(sdBox(p - glm::vec3(-2.2f,0.0f,0.0f), glm::vec3(0.3f,4.0f,2.0f)),1.0f));

    q = glm::vec2(sdBox(p - glm::vec3(-1.0f,0.0f,1.0f), glm::vec3(0.5f,1.0f,0.5f)),1.0f);
    q = max2(q, glm::vec2(-sdBox(p - glm::vec3(-0.5f,0.5f,0.5f), glm::vec3(0.5f,0.7f,0.5f)),3.0f));

    d = min2(d, q);

    //d = min2(d, vec2(sdTorus(p.yxz - vec3(-0.5 + sin(iTime*0.25),1.4,0.5), vec2(1.0, 0.3)),5.0));

    return d;
}
