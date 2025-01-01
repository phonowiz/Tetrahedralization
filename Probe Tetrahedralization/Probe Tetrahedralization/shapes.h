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
#include <fbxsdk.h>


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

glm::vec2 plane( glm::vec3 p, float color_index) {
    return glm::vec2(p.y+1.0,color_index);
}

glm::vec2 do_model( glm::vec3 p, float /*itime*/ ) {
    
    glm::vec2 d = plane(p, 4.0f);

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

glm::vec3 calc_normal( glm::vec3 p, float /*iTime*/ )
{

    const float eps = 0.001;    // precision of the normal computation

    const glm::vec3 v1 = glm::vec3( 1.0f,-1.0f,-1.0f);
    const glm::vec3 v2 = glm::vec3(-1.0f,-1.0f, 1.0f);
    const glm::vec3 v3 = glm::vec3(-1.0f, 1.0f,-1.0f);
    const glm::vec3 v4 = glm::vec3( 1.0f, 1.0f, 1.0f);

    glm::vec3 res = v1*do_model( p + v1*eps, 0.0f ).x +
                        v2*do_model( p + v2*eps, 0.0f ).x +
                        v3*do_model( p + v3*eps, 0.0f ).x +
                        v4*do_model( p + v4*eps, 0.0f ).x ;
    
    if(glm::length(res) == 0.0f)
        return glm::vec3(0.0f);
    
    return normalize(res);
}
