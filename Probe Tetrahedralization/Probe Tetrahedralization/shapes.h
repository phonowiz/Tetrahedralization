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
//#include <fbxsdk.h>
#include <iostream>
#include <tiny_gltf.h>


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



struct triangle
{
    glm::ivec3 indices = {};
    glm::vec3  normal = {};
};

struct asset_vertex_info
{
    std::vector<int>       vertex_material;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> materials;
    std::vector<glm::vec3> normals;
    
    std::vector<triangle> triangles;
};

////based off of https://gamedev.stackexchange.com/questions/77843/fbx-sdk-colors-problem
//void get_fbx_vert_info(FbxMesh* mesh, asset_vertex_info& fbx_vert_info, glm::mat4 world_mat)
//{
//    assert(mesh);
//    int vertex_counter = 0;
//
//    const FbxGeometryElementNormal *normal_element = mesh->GetElementNormal();
//    const FbxGeometryElementVertexColor* color_element = mesh->GetElementVertexColor();
//
//    for (int polyCounter = 0; polyCounter < mesh->GetPolygonCount(); polyCounter++)
//    {
//        int polygonSize = mesh->GetPolygonSize(polyCounter);
//        for (int i = 0; i < polygonSize; i++)
//        {
//            FbxVector4 normal = {};
//            FbxColor color = {};
//            FbxVector4 position = mesh->GetControlPointAt(i);
//
//            switch (normal_element->GetMappingMode())
//            {
//                case FbxGeometryElement::eByControlPoint:
//                {
//                    int control_point_id = mesh->GetPolygonVertex(polyCounter, i);
//                    switch(normal_element->GetReferenceMode())
//                    {
//                        case FbxGeometryElement::eDirect:
//                        {
//                            normal = normal_element->GetDirectArray().GetAt(control_point_id);
//                            color = color_element->GetDirectArray().GetAt(control_point_id);
//                            break;
//                        }
//                        case FbxGeometryElement::eIndexToDirect:
//                        {
//                            normal = normal_element->GetDirectArray().GetAt(control_point_id);
//                            color = color_element->GetDirectArray().GetAt(control_point_id);
//                            break;
//                        }
//                        default:
//                        {
//                            break;
//                        }
//                    }
//                    break;
//                }
//                case FbxGeometryElement::eByPolygonVertex:
//                {
//                    switch(normal_element->GetReferenceMode())
//                    {
//                        case FbxGeometryElement::eDirect:
//                        {
//                            normal = normal_element->GetDirectArray().GetAt(vertex_counter);
//                            color = color_element->GetDirectArray().GetAt(vertex_counter);
//                            break;
//                        }
//                        case FbxGeometryElement::eIndexToDirect:
//                        {
//                            int normalIndex = normal_element->GetIndexArray().GetAt(vertex_counter);
//                            normal = normal_element->GetDirectArray().GetAt(normalIndex);
//
//                            int colorIndex = color_element->GetIndexArray().GetAt(vertex_counter);
//                            color = color_element->GetDirectArray().GetAt(colorIndex);
//                            break;
//                        }
//                        default:
//                        {
//                            break;
//                        }
//                    }
//                    break;
//                }
//                default:
//                {
//                    break;
//                }
//            }
//
//            glm::vec4 glm_normal = glm::vec4(normal[0], normal[1], normal[2], 0.0f);
//            glm::vec4 glm_color  = glm::vec4(color.mRed, color.mGreen, color.mBlue, color.mAlpha);
//            glm::vec4 glm_position = glm::vec4(position[0], position[1], position[2], 1.0f);
//
//            glm_normal = world_mat * glm_normal;
//            glm_position = world_mat * glm_position;
//
//            fbx_vert_info.normals.push_back(glm_normal);
//            fbx_vert_info.colors.push_back(glm_color);
//            fbx_vert_info.positions.push_back(glm_position);
//
//            vertex_counter++;
//        }
//    }
//}


bool get_glb_vertex_info(const char* glb_path, asset_vertex_info& glb_vert_info, glm::mat4 world_mat)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, glb_path);

    if(!warn.empty())
        std::cout << "WARNING: " << err << std::endl;
    if(!err.empty())
        std::cout << "ERROR: " << err << std::endl;
    
    if(!ret)
        return false;
    
    if (!model.materials.empty())
    {
        for(tinygltf::Material& material : model.materials)
        {
            float r = material.pbrMetallicRoughness.baseColorFactor[0];
            float g = material.pbrMetallicRoughness.baseColorFactor[1];
            float b = material.pbrMetallicRoughness.baseColorFactor[2];
            float a = material.pbrMetallicRoughness.baseColorFactor[3];
            
            std::string& name = material.name;
            std::cout << "Material " << name << ": <" << r << ","<< g << "," << b << "," << a << std::endl;
            glb_vert_info.materials.push_back(glm::vec4(r,g,b,a));
        }
        
        
    }
    for (const auto& mesh : model.meshes)
    {
        for (const auto& primitive : mesh.primitives)
        {
            // Check if vertex colors are present
            if (primitive.attributes.count("NORMAL") && primitive.attributes.count("POSITION"))
            {
                int index_start = (int)glb_vert_info.vertex_material.size();
                
                assert(primitive.attributes.count("NORMAL") == primitive.attributes.count("POSITION"));
                const float* norm = nullptr;
                const float* pos = nullptr;
                const uint16_t* triangle_indices = nullptr;
                
                size_t positions = 0, normals = 0, triangles = 0;
                {
                    const tinygltf::Accessor& normal_accessor = model.accessors[primitive.attributes.at("NORMAL")];
                    normals = normal_accessor.count;
                    const tinygltf::BufferView& normal_view = model.bufferViews[normal_accessor.bufferView];
                    norm = reinterpret_cast<const float*>(&model.buffers[normal_view.buffer].data[normal_accessor.byteOffset + normal_view.byteOffset]);
                }
                {
                    const tinygltf::Accessor& position_accessor = model.accessors[primitive.attributes.at("POSITION")];
                    const tinygltf::BufferView& position_view = model.bufferViews[position_accessor.bufferView];
                    pos = reinterpret_cast<const float*>(&model.buffers[position_view.buffer].data[position_accessor.byteOffset + position_view.byteOffset]);
                    positions = position_accessor.count;
                }
                {
                    const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& index_buffer_view = model.bufferViews[index_accessor.bufferView];
                    const tinygltf::Buffer& index_buffer = model.buffers[index_buffer_view.buffer];
                    triangle_indices = reinterpret_cast<const uint16_t*>(&index_buffer.data[index_buffer_view.byteOffset + index_accessor.byteOffset]);
                
                    triangles = index_accessor.count;
                }
                
                assert(positions == normals);
                for (size_t i = 0; i < positions; ++i)
                {
                    int index = (int)glb_vert_info.vertex_material.size();
                    float x = norm[i * 3];
                    float y = norm[i * 3 + 1];
                    float z = norm[i * 3 + 2];
                    
                    float pos_x = pos[i * 3];
                    float pos_y = pos[i * 3 + 1];
                    float pos_z = pos[i * 3 + 2];
                    
                    glb_vert_info.normals.push_back(glm::vec4(x,y,z,0.0));
                    glb_vert_info.vertex_material.push_back(primitive.material);
                    
                    glm::vec4 pos4 = glm::vec4(pos_x, pos_y, pos_z, 1.0f);
                    pos4 = world_mat * pos4;
                    glb_vert_info.positions.push_back(pos4);
                    
                    std::cout << "Nomal (" << index << "): (" << x << ", " << y << ", " << z << ") == Pos (" << pos_x << "," << pos_y << "," << pos_z << ")" << std::endl;
                }
                
                for(int i = 0; i < triangles; i += 3)
                {
                    glm::ivec3 tri(triangle_indices[i + 0] + index_start, triangle_indices[i + 1] + index_start, triangle_indices[i + 2] + index_start);
                    
                    glm::vec3 normal = glm::vec3(0.f);
                    normal += glb_vert_info.normals[tri.x];
                    normal += glb_vert_info.normals[tri.y];
                    normal += glb_vert_info.normals[tri.z];
                    
                    normal /= 3.0f;
                    
                    glm::vec4 normal_h = world_mat * glm::vec4(normal, 0.0f);
                    
                    triangle t;
                    t.indices = tri;
                    t.normal = glm::vec3(normal_h.x, normal_h.y, normal_h.z);
                    
                    glb_vert_info.triangles.push_back(t);
                    
                    std::cout << "=====" << std::endl
                              << glb_vert_info.positions[tri.x].x << "," << glb_vert_info.positions[tri.x].y << "," << glb_vert_info.positions[tri.x].z << std::endl
                              << glb_vert_info.positions[tri.y].x << "," << glb_vert_info.positions[tri.y].y << "," << glb_vert_info.positions[tri.y].z << std::endl
                              << glb_vert_info.positions[tri.z].x << "," << glb_vert_info.positions[tri.z].y << "," << glb_vert_info.positions[tri.z].z << std::endl
                    
                              << "normal: (" << normal.x << ","  << normal.y << "," << normal.z << ")" << std::endl
                    <<  "=====" << std::endl;
                }
            }
            else
            {
                std::cout << "Error, mesh format not supported" << std::endl;
                return false;
            }
        }
    }
    
    printf("#verts: %i, #tris: %i\n", (int)glb_vert_info.positions.size(), (int)glb_vert_info.triangles.size());
    return true;
}
