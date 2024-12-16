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

#include <queue>
#include <iostream>
#include <thread>

std::atomic_uint32_t segment3d::current_id = 0;

class probe_collector
{
public:
    glm::vec3 ro = glm::vec3(0.0f);
    glm::vec3 rd = glm::vec3(0.0f);
    std::atomic<bool>* done = nullptr;
    std::vector<glm::vec3>* probes = nullptr;
    
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
        
        std::vector<glm::vec3>& pr = *probes;
        
        while(std::abs(t) < maxd)
        {
            float h = do_model(ro + rd * t, 0.0f).x;
            
            if(( glm::abs(h) < precis))
            {
                glm::vec3 c = origin + dir * t;
                
                if(!pr.empty())
                {
                    glm::vec3 v = pr.back() - c;
                    if(glm::length(v) > (.4f ))
                        pr.push_back(c);
                }
                else
                    pr.push_back(c);
            }
            
            t += precis;
        }
        
        (*done) = true;
    }
};

#define TOTAL_THREADS (20)
struct collector_manager
{

    std::array<std::thread, TOTAL_THREADS>         threads;
    std::array<probe_collector, TOTAL_THREADS>     collectors;
    std::array<std::atomic<bool>, TOTAL_THREADS>   dones;
    std::array<std::vector<glm::vec3>, TOTAL_THREADS> probes;
    
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

void run_collector(collector_manager* manager, int index)
{
    manager->collectors[index]();
}
void generate_probes(tetgenio& in, std::vector<glm::vec3>& probes )
{
    collector_manager manager;
    
    constexpr float total_width = 10.0f;
    constexpr float total_height = 10.0f;
    
    static_assert(total_width == total_height);
    
    float steps =  1.f;
    
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
    for(float w = -total_width * .5f * .25f; w < total_width * .5f; w += steps)
    {
        for(float h = -total_height * .5f * .25f; h < total_height * .5f; h += steps)
        {
            glm::vec3 ro(w, total_height * .5f, h);
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
            manager.collectors[index].maxd = total_width * 2.f;

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

void light_probes(std::vector<glm::vec3>& probes)
{
    for(glm::vec3 p : probes)
    {
        //glm::vec3 color = glm::vec3(0.0f);
        printf("path tracing probe: %f, %f, %f\n", p.x, p.y, p.z);
        
        sh9_color sh_color = {};
        const float samples = 10;
        for(int i = 0; i < samples; i++)
        {
            glm::vec3 normal = calc_normal(p, 0.0f);
            glm::vec3 rd = random_ray(normal, glm::vec4(p, 0.0f));
            glm::vec3 c = multiple_bounce_path_trace(p, rd);
            
            sh9 sh_normal = sh_evaluate(normal);
            
            sh_color.red =  sh_add(sh_color.red, sh_scale(sh_normal, c.r));
            sh_color.green = sh_add(sh_color.green, sh_scale(sh_normal, c.g));
            sh_color.blue = sh_add(sh_color.blue, sh_scale(sh_normal, c.b));
        }
        const float sh_factor = (4.0f * M_PI) / samples;
        
        sh_color.red = sh_scale(sh_color.red, sh_factor);
        sh_color.green = sh_scale(sh_color.green, sh_factor);
        sh_color.blue = sh_scale(sh_color.blue, sh_factor);
        printf("\tcolor R: %f, %f, %f, %f, %f, %f, %f, %f, %f\n", sh_color.red[0], sh_color.red[1], sh_color.red[2], sh_color.red[3], sh_color.red[4], sh_color.red[5], sh_color.red[6], sh_color.red[7], sh_color.red[8]);
        printf("\tcolor G: %f, %f, %f, %f, %f, %f, %f, %f, %f\n", sh_color.green[0], sh_color.green[1], sh_color.green[2], sh_color.green[3], sh_color.green[4], sh_color.green[5], sh_color.green[6], sh_color.green[7], sh_color.green[8]);
        printf("\tcolor B: %f, %f, %f, %f, %f, %f, %f, %f, %f\n", sh_color.blue[0], sh_color.blue[1], sh_color.blue[2], sh_color.blue[3], sh_color.blue[4], sh_color.blue[5], sh_color.blue[6], sh_color.blue[7], sh_color.blue[8]);
    }
}
int main(int argc, const char * argv[]) {
    
    
    std::vector<glm::vec3> probes;
    tetgenio in, out;
    generate_probes(in, probes);
    light_probes(probes);
    
    printf("total probes count: %d\n", (int)probes.size());
    for(int i = 0; i < probes.size(); ++i)
    {
        std::cout << "d += drawPoint(ro, rd, vec3(" << probes[i].x << "," << probes[i].y << "," << probes[i].z << "));" << std::endl;
    }
    
    // Output the PLC to files 'barin.node' and 'barin.poly'.
    in.save_nodes("barin");
    in.save_poly("barin");

    tetrahedralize("nV", &in, &out);

    //out.tetrahedronlist
    // Output mesh to files 'barout.node', 'barout.ele' and 'barout.face'.
    out.save_nodes("barout");
    out.save_elements("barout");
    out.save_faces("barout");
    out.save_neighbors("barout");
    out.save_poly("barout");
    //out.save_faces2smesh("barout");
    
    return 0;
}
