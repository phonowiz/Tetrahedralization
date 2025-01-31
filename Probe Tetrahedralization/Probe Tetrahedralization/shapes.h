//
//  shapes.h
//  Probe Tetrahedralization
//
//  Created by Rafael Sabino on 12/11/24.
//

#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <array>
#include "mathematics.h"
#include <numbers>
#include <vector>
#include <iostream>
#include <tiny_gltf.h>



struct node_info
{
    glm::mat4 transform = glm::mat4(1.0f);
    
    node_info(){};
    node_info(glm::mat4 trans): transform(trans){};
    bool found = false;

};

node_info get_glb_rotation_info(const tinygltf::Model& model, const tinygltf::Node& node, const tinygltf::Mesh& target_mesh, node_info info)
{
    std::cout << "Node name: " << node.name << std::endl;

    
    if(!node.translation.empty())
    {
        info.transform = glm::translate(info.transform,glm::vec3( node.translation[0], node.translation[1], node.translation[2]));
    }
    
    if(!node.rotation.empty())
    {
        glm::quat quaternion = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        info.transform *= glm::mat4_cast(quaternion);
    }
    
    if(!node.matrix.empty())
    {
        assert(node.matrix.size() == 16);
        glm::mat4 trans = glm::make_mat4(node.matrix.data());
        info.transform *= trans;
    }
    
    if (node.mesh >= 0) {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        std::cout << "Mesh name: " << mesh.name << std::endl;
        
        if(&target_mesh == &mesh)
        {
            printf("found mesh\n");
            info.found = true;
            return info;
        }
    }

    for (const int child_index : node.children)
    {
        node_info temp = get_glb_rotation_info(model, model.nodes[child_index], target_mesh, info);
        if(temp.found)
        {
            info = temp;
            break;
        }
    }
    
    return info;
}

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
        node_info info;
        for(auto& node : model.nodes)
        {
            info = get_glb_rotation_info(model, node, mesh, info);
            
            if(info.found)
                break;
        }

        for (const auto& primitive : mesh.primitives)
        {
            if (primitive.attributes.count("NORMAL") && primitive.attributes.count("POSITION"))
            {
                assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
                
                int index_start = (int)glb_vert_info.positions.size();
                
                assert(primitive.attributes.count("NORMAL") == primitive.attributes.count("POSITION"));
                const float* norm = nullptr;
                const float* pos = nullptr;
                const uint32_t* triangle_indices = nullptr;
                
                size_t positions = 0, normals = 0, triangles = 0;
                {
                    const tinygltf::Accessor& normal_accessor = model.accessors[primitive.attributes.at("NORMAL")];
                    normals = normal_accessor.count;
                    assert(normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                    const tinygltf::BufferView& normal_view = model.bufferViews[normal_accessor.bufferView];
                    norm = reinterpret_cast<const float*>(&model.buffers[normal_view.buffer].data[normal_accessor.byteOffset + normal_view.byteOffset]);
                }
                {
                    const tinygltf::Accessor& position_accessor = model.accessors[primitive.attributes.at("POSITION")];
                    
                    assert(position_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                    const tinygltf::BufferView& position_view = model.bufferViews[position_accessor.bufferView];
                    const tinygltf::Buffer& position_buffer = model.buffers[position_view.buffer];
                    pos = reinterpret_cast<const float*>(&position_buffer.data[position_accessor.byteOffset + position_view.byteOffset]);
                    positions = position_accessor.count;
                }
                {
                    const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& index_buffer_view = model.bufferViews[index_accessor.bufferView];
                    const tinygltf::Buffer& index_buffer = model.buffers[index_buffer_view.buffer];
                    triangle_indices = reinterpret_cast<const uint32_t*>(&index_buffer.data[index_buffer_view.byteOffset + index_accessor.byteOffset]);
                    assert(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
                    triangles = index_accessor.count;
                    
                }
                
                assert(positions == normals);
                for (size_t i = 0; i < positions; i++)
                {
                    float x = norm[i*3 + 0];
                    float y = norm[i*3 + 1];
                    float z = norm[i*3 + 2];
                    
                    float pos_x = pos[i*3 + 0];
                    float pos_y = pos[i*3 + 1];
                    float pos_z = pos[i*3 + 2];
                    
                    glm::vec4 norm4 = glm::vec4(x,y,z, 0.0f);
                    norm4 = world_mat * info.transform * norm4;
                    
                    glb_vert_info.normals.push_back(normalize(glm::vec3(-norm4.x, norm4.y, norm4.z)));
                    glb_vert_info.vertex_material.push_back(primitive.material);
                    
                    glm::vec4 pos4 = glm::vec4(pos_x, pos_y, pos_z, 1.0f);
                    pos4 = world_mat * info.transform * pos4;
                    
                    glb_vert_info.positions.push_back(glm::vec3(-pos4.x, pos4.y, pos4.z));
                    
                    std::cout << "Nomal (" << i << "): (" << x << ", " << y << ", " << z << ") == Pos (" << pos_x << "," << pos_y << "," << pos_z << ")" << std::endl;
                }
                
                for(int i = 0; i < triangles; i += 3)
                {
                    glm::ivec3 tri(triangle_indices[i+0], triangle_indices[i+1], triangle_indices[i+2]);
                    
                    tri.x = (tri.x) + index_start;
                    tri.y = (tri.y) + index_start;
                    tri.z = (tri.z) + index_start;
                    printf("triangle indices info: %i, %i, %i \n", tri.x, tri.y, tri.z);
                    const uint32_t total_pos =  (uint32_t)glb_vert_info.positions.size();
                    assert(   (tri.x < total_pos)
                           && (tri.y < total_pos)
                           && (tri.z < total_pos));
                    
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
