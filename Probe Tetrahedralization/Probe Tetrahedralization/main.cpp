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
    
    float steps =  .85f;
    
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
    for(probe_info& p : probes)
    {
        printf("path tracing probe: %f, %f, %f\n", p.position.x, p.position.y, p.position.z);
        
        //sh9_color sh_color = {};
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
        printf("\tcolor R: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.red[0], p.sh_color.red[1], p.sh_color.red[2], p.sh_color.red[3], p.sh_color.red[4], p.sh_color.red[5], p.sh_color.red[6], p.sh_color.red[7], p.sh_color.red[8]);
        printf("\tcolor G: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.green[0], p.sh_color.green[1], p.sh_color.green[2], p.sh_color.green[3], p.sh_color.green[4], p.sh_color.green[5], p.sh_color.green[6], p.sh_color.green[7], p.sh_color.green[8]);
        printf("\tcolor B: %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f\n", p.sh_color.blue[0], p.sh_color.blue[1], p.sh_color.blue[2], p.sh_color.blue[3], p.sh_color.blue[4], p.sh_color.blue[5], p.sh_color.blue[6], p.sh_color.blue[7], p.sh_color.blue[8]);
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
    }
    
    
    printf("======================================\n"
           "struct tetrahedra\n"
           "{\n"
                "\tivec4   probes;\n"
                "\tivec4   neighbors;\n"

           "};\n"

           "struct probe\n"
           "{\n"
               "\tvec3 position;\n"
               "\tvec4 shr;\n"
               "\tvec4 shg;\n"
               "\tvec4 shb;\n"
           "};\n"
           
           "tetrahedra tetras[%d];\n"
           "probe   probes[%d];\n\n\n"
           "void init_scene(){\n"
           ,
           
           (uint)tetrahedras.size(),
           (uint)probes.size()
           );
    
    
    for(int i = 0; i < probes.size(); ++i)
    {
        printf("\tprobes[%d] = probe(vec3(%f,%f, %f),"
                                                    "vec4(%f, %f, %f, %f), vec4(%f, %f, %f, %f),""vec4(%f, %f, %f, %f));\n",
               i,
               probes[i].position.x, probes[i].position.y, probes[i].position.z,
               
               probes[i].sh_color.red[0], probes[i].sh_color.red[1], probes[i].sh_color.red[2],probes[i].sh_color.red[3],/*probes[i].sh_color.red[4],probes[i].sh_color.red[5],probes[i].sh_color.red[6],probes[i].sh_color.red[7],probes[i].sh_color.red[8],*/
               probes[i].sh_color.green[0], probes[i].sh_color.green[1], probes[i].sh_color.green[2],probes[i].sh_color.green[3],/*probes[i].sh_color.green[4],probes[i].sh_color.green[5],probes[i].sh_color.green[6],probes[i].sh_color.green[7],probes[i].sh_color.green[8],*/
               probes[i].sh_color.blue[0], probes[i].sh_color.blue[1], probes[i].sh_color.blue[2],probes[i].sh_color.blue[3] /*probes[i].sh_color.blue[4],probes[i].sh_color.blue[5],probes[i].sh_color.blue[6],probes[i].sh_color.blue[7],probes[i].sh_color.blue[8]*/
               );
    }
    
    printf("\n\n");
    for( int i = 0; i < out.numberoftetrahedra; ++i)
    {
        printf("\ttetras[%d] = tetrahedra(ivec4(%d, %d, %d, %d), ivec4(%d, %d, %d, %d));\n", i, tetrahedras[i].probes[0], tetrahedras[i].probes[1], tetrahedras[i].probes[2], tetrahedras[i].probes[3],
               tetrahedras[i].neighbors[0], tetrahedras[i].neighbors[1], tetrahedras[i].neighbors[2], tetrahedras[i].neighbors[3]);
    }
    
    printf("}\n\n");
    
    printf("===========================================\n\n");
    
    for(int i = 0; i < probes.size(); ++i)
    {
        std::cout << "d += drawPoint(ro, rd, vec3(" << probes[i].position.x << "," << probes[i].position.y << "," << probes[i].position.z << "), " << i << ");" << std::endl;
    }
}

int find_origin_tetrahedra( std::vector<tetrahedra>& tetrahedras, std::vector<probe_info>& probes)
{
    glm::vec3 target_position(0.0f, .5f, 0.0f);
    int tetra_index = 0;

     while(true)
    {
        glm::vec3 column0 = probes[tetrahedras[tetra_index].probes[0]].position - probes[tetrahedras[tetra_index].probes[3]].position;
        glm::vec3 column1 = probes[tetrahedras[tetra_index].probes[1]].position - probes[tetrahedras[tetra_index].probes[3]].position;
        glm::vec3 column2 = probes[tetrahedras[tetra_index].probes[2]].position - probes[tetrahedras[tetra_index].probes[3]].position;

        glm::mat3 mat(column0, column1, column2);
        
        glm::vec3 abc = glm::inverse(mat) * (target_position - probes[tetrahedras[tetra_index].probes[3]].position);
        float d = 1.0f - abc.x - abc.y - abc.z;
        glm::vec4 coords(abc, d);
        
        const float epsilon = 0.000001f;
        assert( std::abs(1.0f - (coords.x + coords.y + coords.z + coords.w)) <= (epsilon));
        
        float min_coord = std::min(std::min(std::min(coords.x, coords.y), coords.z), coords.w);
        
        if(min_coord >= 0.0f)
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
    
    int origin_tetrahedra = find_origin_tetrahedra(tetrahedras, probes);
    
    // Output mesh to files 'barout.node', 'barout.ele' and 'barout.face'.
    out.save_nodes("barout");
    out.save_elements("barout");
    out.save_faces("barout");
    out.save_neighbors("barout");
    out.save_poly("barout");
    //out.save_faces2smesh("barout");
    
    return 0;
}
