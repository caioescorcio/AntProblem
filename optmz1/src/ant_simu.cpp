#include <vector>
#include <iostream>
#include <random>
#include "fractal_land.hpp"
#include "population.hpp"
#include "pheronome.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "rand_generator.hpp"
#include "time_counter.hpp"

#include <mpi.h>



void advance_time( const fractal_land& land, pheronome& phen, 
                   const position_t& pos_nest, const position_t& pos_food,
                std::size_t& cpteur , Population& pop, int rank, int nbp,
                TimeCounter& counter)
{
    // Local processing of ants
    // Note: each process only has its slice of ants in 'pop' now
    #pragma omp parallel for reduction(+:cpteur)
    for ( size_t i = 0; i < pop.get_size(); ++i )
        pop.advance(i, phen, land, pos_food, pos_nest, cpteur);
    
    // DEBUG: check buffer at food-adjacent cell after ants advance
    /*
    static int adv_dbg = 0;
    if (rank == 0 && adv_dbg < 10) {
        double buf_v1 = phen.get_buffer_value(pos_food.x - 1, pos_food.y, 0);
        double map_v1 = phen(pos_food.x - 1, pos_food.y)[0];
        double buf_food = phen.get_buffer_value(pos_food.x, pos_food.y, 0);
        double map_food = phen(pos_food.x, pos_food.y)[0];
        if (buf_v1 > 0.001) {
            adv_dbg++;
            std::cout << "[ADV_DBG] buf(499,500)=" << buf_v1 << " map(499,500)=" << map_v1
                      << " buf_food=" << buf_food << " map_food=" << map_food << std::endl;
        }
    }
    */
    
    // Synchronize pheromones across all processes (before evaporation)
    counter.start_mpi_sync();
    phen.synchronize();
    counter.end_mpi_sync();
    
    // Pheromone evaporation (each process handles its portion of the map)
    counter.start_evaporation();
    phen.do_evaporation(rank, nbp);
    phen.synchronize_evaporation(); // merge distributed evaporation via MPI_MIN
    counter.end_evaporation();
    
    phen.update(); 
}

int main(int nargs, char* argv[])
{
    MPI_Init(&nargs, &argv);
    int rank, nbp;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nbp);

    TimeCounter counter;
    
    if (rank == 0) SDL_Init( SDL_INIT_VIDEO );
    std::size_t seed = 2026 + rank; // Different seed per rank
    const int nb_ants_total = 5000; // Nombre de fourmis total
    const int nb_ants = nb_ants_total / nbp; // Local ants
    const double eps = 0.8;  // Coefficient d'exploration
    const double alpha=0.7; // Coefficient de chaos
    //const double beta=0.9999; // Coefficient d'évaporation
    const double beta=0.999; // Coefficient d'évaporation
    // Location du nid
    position_t pos_nest{256,256};
    // Location de la nourriture
    position_t pos_food{500,500};
    //const int i_food = 500, j_food = 500;    
    // Génération du territoire 512 x 512 ( 2*(2^8) par direction )
    fractal_land land(8,2,1.,1024);
    double max_val = 0.0;
    double min_val = 0.0;
    
    Population ants(nb_ants);
    ants.set_exploration_coef(eps);
    
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
    for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j ) {
        max_val = std::max(max_val, land(i,j));
        min_val = std::min(min_val, land(i,j));
    }
    double delta = max_val - min_val;
    /* On redimensionne les valeurs de fractal_land de sorte que les valeurs
    soient comprises entre zéro et un */
    for ( fractal_land::dim_t i = 0; i < land.dimensions(); ++i )
    for ( fractal_land::dim_t j = 0; j < land.dimensions(); ++j )  {
        land(i,j) = (land(i,j)-min_val)/delta;
    }
    
    // On va créer des fourmis un peu partout sur la carte :
    auto gen_ant_pos = [&land, &seed] () { return rand_int32(0, land.dimensions()-1, seed); };
    // Create ONLY local ants
    for (size_t i = 0; i < nb_ants; ++i) {
        ants.new_ant({gen_ant_pos(), gen_ant_pos()}, unloaded, seed);
    }
    // On crée toutes les fourmis dans la fourmilière.
    pheronome phen(land.dimensions(), pos_food, pos_nest, alpha, beta);
    
    // Population for display on rank 0: holds ALL nb_ants_total ants gathered from all ranks
    Population disp_ants(nb_ants_total);
    if (rank == 0) {
        disp_ants.set_exploration_coef(eps);
        for (int i = 0; i < nb_ants_total; ++i)
            disp_ants.new_ant({0, 0}, unloaded, 0); // positions filled each frame via MPI_Gather
    }

    Window* win = nullptr;
    Renderer* renderer = nullptr;
    if (rank == 0) {
        win = new Window("Ant Simulation", 2*land.dimensions()+10, land.dimensions()+266);
        renderer = new Renderer( land, phen, pos_nest, pos_food, disp_ants ); // uses all ants
    }
    // Compteur de la quantité de nourriture apportée au nid par les fourmis
    size_t food_quantity = 0;
    SDL_Event event;
    bool cont_loop = true;
    bool not_food_in_nest = true;
    std::size_t it = 0;
    while (cont_loop) {
        if (rank == 0) {
            counter.start_render();
            ++it;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT)
                    cont_loop = false;
            }
            counter.end_render();
        }
        // Broadcast stop condition to other ranks (simplified for now, strictly speaking we should check events)
        MPI_Bcast(&cont_loop, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);

        if (!cont_loop) break;

        // Track food delivered THIS iteration only (delta approach)
        size_t food_before = food_quantity;
        counter.start_advance();
        advance_time( land, phen, pos_nest, pos_food, food_quantity, ants, rank, nbp, counter);
        counter.end_advance();
        
        // Sum only NEW deliveries this step across all processes
        size_t local_delta = food_quantity - food_before;
        size_t global_delta = 0;
        MPI_Allreduce(&local_delta, &global_delta, 1, MPI_UNSIGNED_LONG, MPI_SUM, MPI_COMM_WORLD);
        food_quantity = food_before + global_delta;  // consistent on all ranks

        // Gather all ant positions to rank 0 for display (2 ints per ant: x, y)
        counter.start_mpi_gather();
        std::vector<int> local_pos(nb_ants * 2);
        for (int i = 0; i < nb_ants; ++i) {
            local_pos[2*i]   = ants.get_position(i).x;
            local_pos[2*i+1] = ants.get_position(i).y;
        }
        std::vector<int> all_pos;
        if (rank == 0) all_pos.resize(nb_ants_total * 2);
        MPI_Gather(local_pos.data(), nb_ants * 2, MPI_INT,
                   all_pos.data(),  nb_ants * 2, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank == 0) {
            for (int i = 0; i < nb_ants_total; ++i)
                disp_ants.set_position(i, {all_pos[2*i], all_pos[2*i+1]});
        }
        counter.end_mpi_gather();

        if (rank == 0) {
            counter.start_food();
            renderer->display( *win, food_quantity );  // display() already calls SDL_RenderPresent
            counter.end_food();
            counter.print_averages();
        }
        if ( not_food_in_nest && food_quantity > 0 ) {
            std::cout << "La premiere nourriture est arrivee au nid a l'iteration " << it << std::endl;
            not_food_in_nest = false;
        }
    }
    if (rank == 0) {
        delete renderer;
        delete win;
        SDL_Quit();
    }
    MPI_Finalize();
    return 0;
}