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
void generate_probes(tetgenio& in, std::vector<probe_info>& probes )
{
    collector_manager manager;
    
    constexpr float total_width = 8.0f;
    constexpr float total_height = 8.0f;
    
    static_assert(total_width == total_height);
    
    float steps =  2.0f;
    
    //X PLANE
    for(float w = -total_width * .5f; w < total_width * .5f; w += steps)
    {
        for(float h = -total_height * .5f; h < total_height * .5f; h += steps)
        {
            glm::vec3 ro( -total_width * .5f, w,  h);
            glm::vec3 rd(1.0f, 0.0f, 0.0f);
            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;
            
            int index = 0;
            int count = 0;
            while(true)
            {
                index = count++ % manager.collectors.size();
                if(manager.collectors[index].is_done())
                {
                    if(manager.threads[index].joinable())
                    {
                        manager.threads[index].join();
                    }
                    
                    break;
                }
            }
            
            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;
            manager.collectors[index].maxd = total_width;
        
            manager.threads[index] = std::thread(run_collector, &manager, index);
        }
    }
    
    int count = 0;
    for(std::thread& t : manager.threads)
    {
        if(t.joinable())
            t.join();
        probes.insert(probes.end(), manager.probes[count].begin(), manager.probes[count].end());
        manager.probes[count].clear();
        count++;
    }
    
    //Z PLANE
    for(float w = -total_width * .5f * .5f; w < total_width * .5f; w += steps)
    {
        for(float h = -total_height * .5f * .5f; h < total_height * .5f; h += steps)
        {
            glm::vec3 ro(w, h, total_height * .5f);
            glm::vec3 rd(0.0f, 0.0f, -1.0f);

            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;

            int index = 0;
            int count = 0;
            while(true)
            {
                index = count++ % manager.collectors.size();
                if(manager.collectors[index].is_done())
                {
                    if(manager.threads[index].joinable())
                    {
                        manager.threads[index].join();
                    }
                    break;
                }
            }

            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;
            manager.collectors[index].maxd = total_width;

            manager.threads[index] = std::thread(run_collector, &manager, index);
        }
    }

    count = 0;
    for(std::thread& t : manager.threads)
    {
        if(t.joinable())
            t.join();
        probes.insert(probes.end(), manager.probes[count].begin(), manager.probes[count].end());
        manager.probes[count].clear();
        count++;
    }
    
    //Y IS UP!!!
    for(float w = -total_width * .5f * .8f; w < total_width * .5f * .8f; w += steps)
    {
        for(float h = -total_height * .5f * .8f; h < total_height * .5f * .8f; h += steps)
        {
            glm::vec3 ro(w, total_height, h);
            glm::vec3 rd(0.0f, -1.0f, 0.0f);

            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;
            
            int index = 0;
            int count = 0;
            while(true)
            {
                index = count++ % manager.collectors.size();
                if(manager.collectors[index].is_done())
                {
                    if(manager.threads[index].joinable())
                    {
                        manager.threads[index].join();
                    }
                    break;
                }
            }

            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;
            manager.collectors[index].maxd = 20.0f;

            manager.threads[index] = std::thread(run_collector, &manager, index);
        }
    }

    count = 0;
    for(std::thread& t : manager.threads)
    {
        if(t.joinable())
            t.join();
        probes.insert(probes.end(), manager.probes[count].begin(), manager.probes[count].end());
        count++;
    }

    populate_tetgenio(in, probes);
}

void light_probes(std::vector<probe_info>& probes)
{

    static const size_t Size = 20;
    std::array<std::future<void>, Size>   futures;
    std::array<tracing_result, Size>     results = {};
    int index = 0;
    for(probe_info& p : probes)
    {
        printf("path tracing probe [%d]: %f, %f, %f\n", index++, p.position.x, p.position.y, p.position.z);
        
        const float samples = Size;
        for(int i = 0; i < samples; i++)
        {
            glm::vec3 normal = calc_normal(p.position, 0.0f);
            
            assert(!std::isnan(normal.x) && !std::isnan(normal.y) && !std::isnan(normal.z));
            assert(normal.x != 0 || normal.y != 0 || normal.z != 0 );
            glm::vec3 rd = random_ray(normal, glm::vec4(p.position, 0.0f));
            assert(!std::isnan(rd.x) && !std::isnan(rd.y) && !std::isnan(rd.z));
            
            futures[i] = std::async(std::launch::async, &poor_man_pathtracer, p.position, rd, results.data(), i);
        }
        
        for(int i = 0; i < futures.size(); ++i)
        {
            futures[i].wait();
            
            sh9 sh_dir = sh_evaluate(results[i].dir);
            
            glm::vec3 probe_color = results[i].color;
            if(probe_color != glm::vec3(0.0f))
                printf("lighting probe with color <%f, %f, %f>\n", probe_color.r, probe_color.g, probe_color.b);
            p.sh_color.red =  sh_add(p.sh_color.red, sh_scale(sh_dir, results[i].color.r));
            p.sh_color.green = sh_add(p.sh_color.green, sh_scale(sh_dir, results[i].color.g));
            p.sh_color.blue = sh_add(p.sh_color.blue, sh_scale(sh_dir, results[i].color.b));
            
            
        }
        const float sh_factor = (4.0f * M_PI) / samples;
        
        p.sh_color.red = sh_scale(p.sh_color.red, sh_factor);
        p.sh_color.green = sh_scale(p.sh_color.green, sh_factor);
        p.sh_color.blue = sh_scale(p.sh_color.blue, sh_factor);
        
        for(int i = 0; i < 9; ++i)
        {
            assert(!std::isnan(p.sh_color.red[i]));
            assert(!std::isnan(p.sh_color.green[i]));
            assert(!std::isnan(p.sh_color.blue[i]));
        }
        //printf("\tcolor R: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.red[0], p.sh_color.red[1], p.sh_color.red[2], p.sh_color.red[3], p.sh_color.red[4], p.sh_color.red[5], p.sh_color.red[6], p.sh_color.red[7], p.sh_color.red[8]);
        //printf("\tcolor G: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.green[0], p.sh_color.green[1], p.sh_color.green[2], p.sh_color.green[3], p.sh_color.green[4], p.sh_color.green[5], p.sh_color.green[6], p.sh_color.green[7], p.sh_color.green[8]);
        //printf("\tcolor B: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.blue[0], p.sh_color.blue[1], p.sh_color.blue[2], p.sh_color.blue[3], p.sh_color.blue[4], p.sh_color.blue[5], p.sh_color.blue[6], p.sh_color.blue[7], p.sh_color.blue[8]);
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
            glm::vec3 rd = calc_normal(p.position, 0.0);
            //glm::vec3 rd = glm::normalize(ro - p.position);
            
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
    
    int origin_tetra = find_origin_tetrahedra(tetrahedras, probes);
    
    printf("Common ======================================\n"
           
           "float packfragcoord2 (vec2 p, vec2 s) {\n"
               "\treturn floor(p.y) * s.x + p.x;\n"
           "}\n"
           "vec2 unpackfragcoord2 (float p, vec2 s) {\n"
               "\tfloat x = mod(p, s.x);\n"
               "\tfloat y = (p - x) / s.x + 0.5;\n"
               "\treturn vec2(x,y);\n"
           "}\n"
           "ivec2 unpackfragcoord2 (int p, ivec2 s) {\n"
               "\tint x = p %% s.x;\n"
               "\tint y = (p - x) / s.x;\n"
               "\treturn ivec2(x,y);\n"
           "}\n"
           "float packfragcoord3 (vec3 p, vec3 s) {\n"
               "\treturn floor(p.z) * s.x * s.y + floor(p.y) * s.x + p.x;\n"
           "}\n"
           "int packfragcoord3 (ivec3 p, ivec3 s) {\n"
               "\treturn p.z * s.x * s.y + p.y * s.x + p.x;\n"
           "}\n"
           "vec3 unpackfragcoord3 (float p, vec3 s) {\n"
               "\tfloat x = mod(p, s.x);\n"
               "\tfloat y = mod((p - x) / s.x, s.y);\n"
               "\tfloat z = (p - x - floor(y) * s.x) / (s.x * s.y);\n"
               "\treturn vec3(x,y+0.5,z+0.5);\n"
           "}\n"

           "struct tetrahedra\n"
           "{\n"
                "\tivec4   probes;\n"
                "\tivec4   neighbors;\n"
                "\tmat3    matrix;\n"
           "};\n"
           
           "const int origin_tetra = %d;\n"
           "struct probe\n"
           "{\n"
               "\tvec4 shr;\n"
               "\tvec4 shg;\n"
               "\tvec4 shb;\n"
               "\tvec3 position;\n"
           "};\n"
           "const uint total_tetras = %d;\n"
           "const uint total_probes = %d;\n"
           ,
           origin_tetra,
           (uint)(tetrahedras.size()),
           (uint)probes.size()
           );
    
    
    printf("First Pass==================================\n");
    
//    printf("#define vec4 float4\n");
//    printf("#define vec3 float3\n");
//    printf("#define vec2 float2\n");
    
    printf("\tint index = int(packfragcoord2(fragCoord.xy, iResolution.xy));\n");
    printf("\tfragColor = vec4(0.0f);\n");
    printf("\tif(index <  total_probes * 4){\n");

    
    static const sh9 zero =  {};
    for(int i = 0; i < probes.size(); ++i)
    {
        if(probes[i].sh_color.red != zero)
        {
            printf("\t\tif(index == %i)\n", (i * 4) + 0);
            printf("\t\t\tfragColor = vec4(%f, %f, %f, %f);\n", probes[i].sh_color.red[0], probes[i].sh_color.red[1], probes[i].sh_color.red[2], probes[i].sh_color.red[3]);
        }
        if(probes[i].sh_color.green != zero)
        {
            printf("\t\tif(index == %i)\n", (i * 4) + 1);
            printf("\t\t\tfragColor = vec4(%f, %f, %f, %f);\n", probes[i].sh_color.green[0], probes[i].sh_color.green[1], probes[i].sh_color.green[2], probes[i].sh_color.green[3]);
        }
        
        if(probes[i].sh_color.blue != zero)
        {
            printf("\t\tif(index == %i)\n", (i * 4) + 2);
            printf("\t\t\tfragColor = vec4(%f, %f, %f, %f);\n", probes[i].sh_color.blue[0], probes[i].sh_color.blue[1], probes[i].sh_color.blue[2], probes[i].sh_color.blue[3]);
        }

        if(probes[i].position != glm::vec3(0.0f))
        {
            printf("\t\tif(index == %i)\n", (i * 4) + 3);
            printf("\t\t\tfragColor = vec4(%f, %f, %f, 0.0f);\n", probes[i].position[0], probes[i].position[1], probes[i].position[2]);
        }
    }
    printf("\t}\n");
    
    printf("\telse{\n");
    for( int i = 0; i < out.numberoftetrahedra; ++i)
    {
        printf("\t\tif(index == %i)\n", (i * 2) + 0 + (int)probes.size() * 4);
        printf("\t\t\tfragColor = vec4(%i.f, %i.f, %i.f, %i.f);\n", tetrahedras[i].probes[0], tetrahedras[i].probes[1], tetrahedras[i].probes[2], tetrahedras[i].probes[3]);
        printf("\t\tif(index == %i)\n", (i * 2) + 1 + (int)probes.size() * 4);
        printf("\t\t\tfragColor = vec4(%i.f, %i.f, %i.f, %i.f);\n", tetrahedras[i].neighbors[0], tetrahedras[i].neighbors[1], tetrahedras[i].neighbors[2], tetrahedras[i].neighbors[3]);
    }
    
    printf("\t}\n\n");
    
    printf("===========================================\n\n");
    
    for(int i = 0; i < probes.size(); ++i)
    {
        int tetra_index = get_tetrahedra(probes[i].position, tetrahedras, probes);
        std::cout << "\t\td += drawPoint(ro, rd, vec3(" << probes[i].position.x << "," << probes[i].position.y << "," << probes[i].position.z << "), " << i << "," << tetra_index << ");" << std::endl;
    }
}

int main(int argc, const char * argv[]) {
    
    
    std::vector<probe_info> probes;
    std::vector<tetrahedra> tetrahedras;
    tetgenio in, out;
    generate_probes(in, probes);
    light_probes(probes);
    
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
