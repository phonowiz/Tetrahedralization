//
//  main.cpp
//  Probe Tetrahedralization
//
//  Created by Rafael Sabino on 12/5/24.
//
#include "tetgen.h"
#include "mathematics.h"
#include "helpers.h"
#include "shapes.h"

#include <future>
#include <thread>
#include <queue>
#include <iostream>
#include <chrono>
#include <ctime>

std::atomic_uint32_t segment3d::current_id = 0;

void run_collector(collector_manager* manager, int index)
{
    manager->collectors[index]();
}


void light_probes(std::vector<probe_info>& probes, const asset_vertex_info& vert_info)
{

    static const size_t Size = 20;
    std::array<std::future<void>, Size>   futures;
    std::array<tracing_result, Size>     results = {};
    int index = 0;
    for(probe_info& p : probes)
    {
        printf("path tracing probe [%d]: %f, %f, %f\n", index++, p.position.x, p.position.y, p.position.z);
        
        const float samples = Size;
        const float max_trace = 20.f;
        for(int i = 0; i < samples; i++)
        {
            glm::vec3 normal = p.normal;
            
            assert(!std::isnan(normal.x) && !std::isnan(normal.y) && !std::isnan(normal.z));
            assert(normal.x != 0 || normal.y != 0 || normal.z != 0 );
            glm::vec3 rd = random_ray(normal, glm::vec4(p.position, 0.0f)) * max_trace;
            assert(!std::isnan(rd.x) && !std::isnan(rd.y) && !std::isnan(rd.z));
            
            futures[i] = std::async(std::launch::async, &poor_man_pathtracer_tri, p.position, rd, results.data(), vert_info, i);
        }
        
        for(int i = 0; i < futures.size(); ++i)
        {
            futures[i].wait();
            
            sh9 sh_dir = sh_evaluate(p.normal);
            
            p.sh_color.red =  sh_add(p.sh_color.red, sh_scale(sh_dir, results[i].color.r));
            p.sh_color.green = sh_add(p.sh_color.green, sh_scale(sh_dir, results[i].color.g));
            p.sh_color.blue = sh_add(p.sh_color.blue, sh_scale(sh_dir, results[i].color.b));
            
            
        }
        const float sh_factor = (2.0f * M_PI) / samples;
        
        p.sh_color.red = sh_scale(p.sh_color.red, sh_factor);
        p.sh_color.green = sh_scale(p.sh_color.green, sh_factor);
        p.sh_color.blue = sh_scale(p.sh_color.blue, sh_factor);
        
        for(int i = 0; i < 9; ++i)
        {
            assert(!std::isnan(p.sh_color.red[i]));
            assert(!std::isnan(p.sh_color.green[i]));
            assert(!std::isnan(p.sh_color.blue[i]));
        }
        printf("\tcolor R: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.red[0], p.sh_color.red[1], p.sh_color.red[2], p.sh_color.red[3], p.sh_color.red[4], p.sh_color.red[5], p.sh_color.red[6], p.sh_color.red[7], p.sh_color.red[8]);
        printf("\tcolor G: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.green[0], p.sh_color.green[1], p.sh_color.green[2], p.sh_color.green[3], p.sh_color.green[4], p.sh_color.green[5], p.sh_color.green[6], p.sh_color.green[7], p.sh_color.green[8]);
        printf("\tcolor B: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.blue[0], p.sh_color.blue[1], p.sh_color.blue[2], p.sh_color.blue[3], p.sh_color.blue[4], p.sh_color.blue[5], p.sh_color.blue[6], p.sh_color.blue[7], p.sh_color.blue[8]);
    }
}


int get_tetrahedra(glm::vec3 target_position, std::vector<tetrahedra>& tetrahedras, std::vector<probe_info>& probes)
{
    int tetra_index = 0;

    while(true)
    {
        glm::vec3 abc = tetrahedras[tetra_index].matrix * (target_position - probes[tetrahedras[tetra_index].probes[3]].position);
        float d = 1.0f - abc.x - abc.y - abc.z;
        glm::vec4 coords(abc, d);
        
        assert( std::abs(1.0f - (coords.x + coords.y + coords.z + coords.w)) <= (precis));
        
        float min_coord = std::min(std::min(std::min(coords.x, coords.y), coords.z), coords.w);
        
        if(min_coord  >=  -precis)
            break;
        int old_tetra = tetra_index;
        tetra_index = tetrahedras[old_tetra].neighbors[3];
        
        if(min_coord == coords.x)
            tetra_index = tetrahedras[old_tetra].neighbors[0];
        else if(min_coord == coords.y)
            tetra_index = tetrahedras[old_tetra].neighbors[1];
        else if(min_coord == coords.z)
            tetra_index = tetrahedras[old_tetra].neighbors[2];
    }
    
    return tetra_index;
}
int find_origin_tetrahedra( std::vector<tetrahedra>& tetrahedras, std::vector<probe_info>& probes)
{
    glm::vec3 target_position(0.0f, .5f, 0.0f);
    return get_tetrahedra(target_position, tetrahedras, probes);
}


glm::vec4 cosine_lobe_sh = glm::vec4(0.8862f, -1.0233f, 1.0233f, -1.0233f);
glm::vec4 sh_base = glm::vec4(0.2821f, -0.4886f, 0.4886f, -0.4886f);

glm::vec4 sh_normal(glm::vec3 normal)
{
    return glm::vec4(1.0, normal.x,normal.y, normal.z)  ;
}

glm::vec4 sh_lobe_rotate(glm::vec3 normal)
{
    return sh_normal(normal) * cosine_lobe_sh;
}

float sh_integrate( glm::vec4 sh_color, glm::vec3 normal)
{
    
    return glm::max(0.f,dot(sh_lobe_rotate(normal), sh_color));
}

glm::vec3 sh_probe_color(glm::vec4 shr, glm::vec4 shg, glm::vec4 shb, glm::vec3 normal)
{
    return glm::vec3(
        sh_integrate(shr, normal),
        sh_integrate(shg, normal),
        sh_integrate(shb, normal)
    );
}

glm::vec3 get_color(glm::vec3 dir, glm::vec3 world_pos, std::vector<tetrahedra>& tetras, std::vector<probe_info>& probes)
{
    int tetra_index = get_tetrahedra(world_pos, tetras, probes);
    
    glm::vec4 shr = glm::vec4(0);
    glm::vec4 shg = glm::vec4(0);
    glm::vec4 shb = glm::vec4(0);
    
    if(tetra_index != -1)
    {
        glm::ivec4 neighbors = tetras[tetra_index].probes;
        
        glm::vec3 abc = tetras[tetra_index].matrix * (world_pos -  probes[tetras[tetra_index].probes[3]].position);
        float d = 1.0f - abc.x - abc.y - abc.z;
        glm::vec4 coords = glm::vec4(abc, d);
        
        for(int i = 0; i < 4; ++i)
        {
            if(neighbors[i] != -1)
            {
  
                float weight = coords[i];
                probe_info &p = probes[ neighbors[i] ];
                
                glm::vec4 temp_shr = glm::vec4(p.sh_color.red[0], p.sh_color.red[1], p.sh_color.red[2], p.sh_color.red[3]);
                glm::vec4 temp_shg = glm::vec4(p.sh_color.green[0], p.sh_color.green[1], p.sh_color.green[2], p.sh_color.green[3]);
                glm::vec4 temp_shb = glm::vec4(p.sh_color.blue[0], p.sh_color.blue[1], p.sh_color.blue[2], p.sh_color.blue[3]);
                
                shr += temp_shr * weight;
                shg += temp_shg * weight;
                shb += temp_shb * weight;
            }
        }
    }
    return sh_probe_color(shr, shg, shb, -dir);
}

void test_unprojection(std::vector<tetrahedra>& tetrahedras, std::vector<probe_info>& probes)
{
    time_t start_time = time(nullptr);

    printf("COLOR TEST!\n");
    static const intmax_t total_time = 10;
    for(int i = 0; i < 1000; ++i)
    {
        time_t current_time = time(nullptr);
        
        float t = float((current_time - start_time)) / float(total_time);
        
        glm::vec3 ro = glm::vec3(8.f*sin(t) - .2f, 2.f + sin(t), -8.f*cos(t) + .2f);
        int j =0;
        for(probe_info& p : probes)
        {
            glm::vec3 rd = p.normal;
            glm::vec3 color = get_color(rd, p.position, tetrahedras, probes);
            
            printf("color picked for probe [%d]: %.7f, %.7f, %.7f\n", j, color.r, color.g, color.b);
            ++j;
        }
    }
}

void write_probe_array(std::vector<probe_info>& probes, tetgenio& out, std::vector<tetrahedra>& tetrahedras)
{
    assert(out.firstnumber == 0 && "this will not work unless first number is zero");
    assert(out.numberofcorners == 4);
    for(int i = 0; i < out.numberoftetrahedra; ++i)
    {
        tetrahedras.push_back(tetrahedra());
        tetrahedras[i].neighbors[0] = out.neighborlist[i * 4];
        tetrahedras[i].neighbors[1] = out.neighborlist[i * 4 + 1];
        tetrahedras[i].neighbors[2] = out.neighborlist[i * 4 + 2];
        tetrahedras[i].neighbors[3] = out.neighborlist[i * 4  + 3];
        for(int j = 0; j < out.numberofcorners; ++j)
            tetrahedras[i].probes[j] = out.tetrahedronlist[i * out.numberofcorners + j];
        
        glm::vec3 column0 = probes[tetrahedras[i].probes[0]].position - probes[tetrahedras[i].probes[3]].position;
        glm::vec3 column1 = probes[tetrahedras[i].probes[1]].position - probes[tetrahedras[i].probes[3]].position;
        glm::vec3 column2 = probes[tetrahedras[i].probes[2]].position - probes[tetrahedras[i].probes[3]].position;
        glm::mat3 mat(column0, column1, column2);
        
        tetrahedras[i].matrix = glm::inverse(mat);
    }
    
    printf("Baked Data========================\n");
    
    printf("const lightProbes = Float32Array([");
    for(int i = 0; i < probes.size(); ++i)
    {
        if(i != 0)
            printf(", ");
        printf("%f, %f, %f, %f", probes[i].sh_color.red[0], probes[i].sh_color.red[1], probes[i].sh_color.red[2], probes[i].sh_color.red[3]);
    }
    printf("]);\n");
    
    printf("const tetrahedras = new Int16Array([");
    for( int i = 0; i < out.numberoftetrahedra; ++i)
    {
        if( i != 0 )
            printf(", ");
        printf("%d, %d, %d, %d", tetrahedras[i].probes[0], tetrahedras[i].probes[1], tetrahedras[i].probes[2], tetrahedras[i].probes[3]);
    }
    
    printf("]);\n");
    
    printf("const neighbors = new Int16Array([");
    for( int i = 0; i < out.numberoftetrahedra; ++i)
    {
        if( i != 0 )
            printf(", ");
        printf("%d, %d, %d, %d", tetrahedras[i].neighbors[0], tetrahedras[i].neighbors[1], tetrahedras[i].neighbors[2], tetrahedras[i].neighbors[3]);
    }
    
    printf("]);\n");
    
    
    printf("const lightProbesBuffer = new BABYLON.Buffer(engine, lightProbes, false, false, 4);\n");
    printf("const tetrahedrasBuffer = new BABYLON.Buffer(engine, tetrahedras, false, false, 4);\n");
    printf("const neighborsBuffer = new BABYLON.Buffer(engine, neighbors, false, false, 4);\n");
    
    
    printf("Second Pass==================================\n");
    printf("var diameter = 0.08;\n");
    for(int i = 0; i < probes.size(); ++i)
    {
        printf("\tsphere%i = BABYLON.MeshBuilder.CreateSphere(\"sphere%i\", {diameter: diameter, segments: 5}, scene);\n", i,i);
        printf("\tsphere%i.position.x= %f, sphere%i.position.y = %f, sphere%i.position.z = %f;\n",
               i, probes[i].position[0], i, probes[i].position[1], i, probes[i].position[2]);
        
        printf("\tsphere%i.material = simpleMat.clone(\"sphere%i\");\n", i,i);
        printf("\tsphere%i.material.setArray3(\"probePosition\", sphere%i.position);\n",i,i);
        printf("\tsphere%i.material.setArray3(\"probeNormal\", new Float32Array([%f, %f, %f]));\n",i, probes[i].normal.x, probes[i].normal.y, probes[i].normal.z);
        printf("\tsphere%i.material.setArray4(\"shColorR\", new Float32Array([%f, %f, %f, %f]));\n", i, probes[i].sh_color.red[0], probes[i].sh_color.red[1], probes[i].sh_color.red[2], probes[i].sh_color.red[3]);
        printf("\tsphere%i.material.setArray4(\"shColorG\", new Float32Array([%f, %f, %f, %f]));\n", i, probes[i].sh_color.green[0], probes[i].sh_color.green[1], probes[i].sh_color.green[2], probes[i].sh_color.green[3]);
        printf("\tsphere%i.material.setArray4(\"shColorB\", new Float32Array([%f, %f, %f, %f]));\n", i, probes[i].sh_color.blue[0], probes[i].sh_color.blue[1], probes[i].sh_color.blue[2], probes[i].sh_color.blue[3]);
    }
    
    printf("==================================\n");

}

int main(int argc, const char * argv[]) {
    
    
    asset_vertex_info vertex_info = {};
    glm::mat4 world_mat = glm::mat4(1.0f);
    get_glb_vertex_info("assets/cornell_box/cornell_box-_original.glb", vertex_info, world_mat);
    
    std::vector<probe_info> probes;
    std::vector<tetrahedra> tetrahedras;
    tetgenio in, out;
    generate_probes(in, probes, vertex_info);
    
    light_probes(probes, vertex_info);
    
    printf("total probes count: %d\n", (int)probes.size());
    // Output the PLC to files 'barin.node' and 'barin.poly'.
    in.save_nodes("barin");
    in.save_poly("barin");

    tetrahedralize("nV", &in, &out);

    write_probe_array(probes, out, tetrahedras);
    //test_unprojection(tetrahedras, probes);
    // Output mesh to files 'barout.node', 'barout.ele' and 'barout.face'.
    out.save_nodes("barout");
    out.save_elements("barout");
    out.save_faces("barout");
    out.save_neighbors("barout");
    out.save_poly("barout");
    //out.save_faces2smesh("barout");
    
    return 0;
}
