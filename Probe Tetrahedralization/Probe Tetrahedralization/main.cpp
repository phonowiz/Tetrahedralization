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

//static constexpr float maxd = 20.0f;        // max trace distance
static constexpr float precis = 0.00001;    // precission of the intersection

float calc_intersection( glm::vec3 ro, glm::vec3 rd, float maxd, bool& collision)
{
    collision = false;
    float h = precis * 2.0f;
    float t = 0.0;
    float res = -1.0;
    for( int i=0; (i < 90) && (std::abs(t) < maxd); i++)          // max number of raymarching iterations is 90
    {
        collision = (h >= 0.0f) && ( h < precis);
        if(collision)
            break;
        
        h = do_model( ro+rd*t, 0 ).x;
        
        t += std::abs(h);
    }
    res = t;
    return res;
}


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
    
    bool is_done(){ return *done;}
    
    //std::vector<glm::vec3>* probes;
    
    void operator()()
    {
        (*done) = false;
        float t = 0.0f;
        glm::vec3 origin = ro;
        glm::vec3 dir = rd;
        
        std::vector<glm::vec3>& pr = *probes;
        pr.clear();
        
        while(std::abs(t) < maxd)
        {
            bool collision = false;
            t = calc_intersection(origin, dir, maxd, collision);
            
            if(collision)
            {
                glm::vec3 c = origin + dir * t;
                
                if(!pr.empty())
                {
                    //glm::vec3 v = pr.front() - c;
                    //if(v.length() < (.1))
                        pr.push_back(c);
                }
                else
                    pr.push_back(c);
                
//                t = std::abs(t) + precis;
            }
//            else
//                t = std::abs(t) + precis;
            
            origin = origin + dir * std::abs(t);
            maxd -= std::abs(t);
            t = std::abs(t) + precis;
            
            maxd = std::max(maxd, 0.0f);

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


void generate_probes(tetgenio& in, std::vector<glm::vec3>& probes )
{
    collector_manager manager;
    
    constexpr float total_width = 10.0f;
    constexpr float total_height = 10.0f;
    
    static_assert(total_width == total_height);
    
    float steps =  .3f;

    
    //X PLANE
    for(float w = -total_width * .5f; w < total_width * .5f; w += steps)
    {
        for(float h = -total_height * .5f; h < total_height * .5f; h += steps)
        {
            glm::vec3 ro(-total_width, w, h);
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
                    probes.insert(probes.end(), manager.probes[index].begin(), manager.probes[index].end());
                    manager.probes[index].clear();
                    break;
                }
            }
            
            manager.collectors[index].ro = ro;
            manager.collectors[index].rd = rd;
            manager.collectors[index].maxd = total_width;
        
            manager.threads[index] = std::thread(manager.collectors[index]);
        }
    }
    
    int count = 0;
    for(std::thread& t : manager.threads)
    {
        if(t.joinable())
            t.join();
        probes.insert(probes.end(), manager.probes[count].begin(), manager.probes[count].end());
        count++;
    }
    
    
//    //Z PLANE
//    for(float w = -total_width * .5f; w < total_width * .5f; w += steps)
//    {
//        for(float h = -total_height * .5f; h < total_height * .5f; h += steps)
//        {
//            glm::vec3 ro(w, h, total_height);
//            glm::vec3 rd(0.0f, 0.0f, -1.0f);
//
//            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;
//
//            //glm::vec3 rd(-1.0f, 0.0f, 0.0f);
//
//            int index = 0;
//            int count = 0;
//            while(true)
//            {
//                index = count++ % manager.collectors.size();
//                if(manager.collectors[index].is_done())
//                {
//                    if(manager.threads[index].joinable())
//                    {
//                        manager.threads[index].join();
//                    }
//                    probes.insert(probes.end(), manager.probes[index].begin(), manager.probes[index].end());
//                    manager.probes[index].clear();
//                    break;
//                }
//            }
//
//            manager.collectors[index].ro = ro;
//            manager.collectors[index].rd = rd;
//            manager.collectors[index].maxd = total_width * 2.f;
//
//            manager.threads[index] = std::thread(manager.collectors[index]);
//        }
//    }
//
//    count = 0;
//    for(std::thread& t : manager.threads)
//    {
//        if(t.joinable())
//            t.join();
//        probes.insert(probes.end(), manager.probes[count].begin(), manager.probes[count].end());
//        count++;
//    }
//
//
//    //Y IS UP!!!
//    for(float w = -total_width * .5f; w < total_width * .5f; w += steps)
//    {
//        for(float h = -total_height * .5f; h < total_height * .5f; h += steps)
//        {
//            glm::vec3 ro(w, total_height, h);
//            glm::vec3 rd(0.0f, -1.0f, 0.0f);
//
//            std::cout << "checking <" << ro.x << "," << ro.y << "," << ro.z << ">"  << std::endl;
//
//            //glm::vec3 rd(-1.0f, 0.0f, 0.0f);
//
//            int index = 0;
//            int count = 0;
//            while(true)
//            {
//                index = count++ % manager.collectors.size();
//                if(manager.collectors[index].is_done())
//                {
//                    if(manager.threads[index].joinable())
//                    {
//                        manager.threads[index].join();
//                    }
//                    probes.insert(probes.end(), manager.probes[index].begin(), manager.probes[index].end());
//                    manager.probes[index].clear();
//                    break;
//                }
//            }
//
//            manager.collectors[index].ro = ro;
//            manager.collectors[index].rd = rd;
//            manager.collectors[index].maxd = total_width * 2.f;
//
//            manager.threads[index] = std::thread(manager.collectors[index]);
//        }
//    }
//
//    count = 0;
//    for(std::thread& t : manager.threads)
//    {
//        if(t.joinable())
//            t.join();
//        probes.insert(probes.end(), manager.probes[count].begin(), manager.probes[count].end());
//        count++;
//    }

    populate_tetgenio(in, probes);
}
int main(int argc, const char * argv[]) {
    
    
    std::vector<glm::vec3> probes;
    tetgenio in, out;
    generate_probes(in, probes);
    
    for(int i = 0; i < probes.size(); ++i)
    {
        std::cout << "d += drawPoint(ro, rd, vec3(" << probes[i].x << "," << probes[i].y << "," << probes[i].z << "));" << std::endl;
    }

    //generate_input_tetgen(in);
    
    // Output the PLC to files 'barin.node' and 'barin.poly'.
    in.save_nodes("barin");
    in.save_poly("barin");

    tetrahedralize("nVfc", &in, &out);

    // Output mesh to files 'barout.node', 'barout.ele' and 'barout.face'.
    out.save_nodes("barout");
    out.save_elements("barout");
    out.save_faces("barout");
    out.save_neighbors("barout");
    out.save_poly("barout");
    

    
    return 0;
}
