// main.cpp

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>
#include <limits>
#include <SDL2/SDL.h>
#include <mpi.h>

#include "population.hpp"
#include "fractal_land.hpp"
#include "pheronome.hpp"
#include "rand_generator.hpp"
#include "time_counter.hpp"

// Tags for land and pheromone communications
constexpr int land_tag      = 100;
constexpr int phero_food_tag= 300; // pheromone component 0 (towards food)
constexpr int phero_nest_tag= 301; // pheromone component 1 (towards nest)

static void advance_time(const fractal_land& land,
                         pheronome& phen,
                         const position_t& pos_nest_local_safe,
                         const position_t& pos_food_local_safe,
                         std::size_t& cpteur,
                         Population& pop)
{
    // Advance each ant by one step
    for (std::size_t i = 0; i < pop.get_size(); ++i) {
        pop.advance(i, phen, land, pos_food_local_safe, pos_nest_local_safe, cpteur);
    }

    // Global evaporation on the buffer, then commit update
    phen.do_evaporation();
    phen.update();
}





//Pheromones in ghost cells exchange 
static void update_ghost_pheromones(pheronome& phen,const fractal_land& land,MPI_Comm cart_workers,int overlap)
{
    if (cart_workers == MPI_COMM_NULL || overlap <= 0) return;

    int nbr_xm = MPI_PROC_NULL, nbr_xp = MPI_PROC_NULL;
    int nbr_ym = MPI_PROC_NULL, nbr_yp = MPI_PROC_NULL;

    // direction 0 = x, direction 1 = y
    MPI_Cart_shift(cart_workers, 0, 1, &nbr_xm, &nbr_xp);
    MPI_Cart_shift(cart_workers, 1, 1, &nbr_ym, &nbr_yp);

    const int dimx    = static_cast<int>(land.dimensions());
    const int dimy    = static_cast<int>(land.height());
    const int owned_w = static_cast<int>(land.owned_dimensions());
    const int owned_h = static_cast<int>(land.owned_height());

    // Actual owned block origin inside local storage (with halos)
    const int ox = static_cast<int>(land.inner_x_begin());
    const int oy = static_cast<int>(land.inner_y_begin());

    if (owned_w <= 0 || owned_h <= 0) return;
    if (ox < 0 || oy < 0 || ox + owned_w > dimx || oy + owned_h > dimy) return;

    const int halo_left   = ox;
    const int halo_right  = dimx - (ox + owned_w);
    const int halo_top    = oy;
    const int halo_bottom = dimy - (oy + owned_h);

    const int wL = std::min(overlap, halo_left);
    const int wR = std::min(overlap, halo_right);
    const int hT = std::min(overlap, halo_top);
    const int hB = std::min(overlap, halo_bottom);

    // Packs a rectangular region into a contiguous buffer: [food, nest, food, nest, ...]
    auto pack_rect = [&](int x0, int y0, int w, int h, std::vector<double>& out) {
        out.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 2ULL);
        std::size_t k = 0;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const auto& c = phen(static_cast<unsigned long>(x0 + x),
                                     static_cast<unsigned long>(y0 + y));
                out[k++] = c[0]; // food pheromone
                out[k++] = c[1]; // nest pheromone
            }
        }
    };

    // Unpacks contiguous buffer back into a rectangular region
    auto unpack_rect = [&](int x0, int y0, int w, int h, const std::vector<double>& in) {
        std::size_t k = 0;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                auto& c = phen(static_cast<unsigned long>(x0 + x),
                               static_cast<unsigned long>(y0 + y));
                c[0] = in[k++];
                c[1] = in[k++];
            }
        }
    };

    constexpr int tag_x_to_minus = 410;
    constexpr int tag_x_to_plus  = 411;
    constexpr int tag_y_to_minus = 412;
    constexpr int tag_y_to_plus  = 413;

    std::vector<double> send_buf, recv_buf;

    // Exchange with left neighbor: send left owned band, receive into left ghost band
    if (nbr_xm != MPI_PROC_NULL && wL > 0) {
        pack_rect(ox, oy, wL, owned_h, send_buf);
        recv_buf.resize(send_buf.size());
        MPI_Sendrecv(send_buf.data(), static_cast<int>(send_buf.size()), MPI_DOUBLE, nbr_xm, tag_x_to_minus,
                     recv_buf.data(), static_cast<int>(recv_buf.size()), MPI_DOUBLE, nbr_xm, tag_x_to_plus,
                     cart_workers, MPI_STATUS_IGNORE);
        unpack_rect(ox - wL, oy, wL, owned_h, recv_buf);
    }

    // Exchange with right neighbor: send right owned band, receive into right ghost band
    if (nbr_xp != MPI_PROC_NULL && wR > 0) {
        pack_rect(ox + owned_w - wR, oy, wR, owned_h, send_buf);
        recv_buf.resize(send_buf.size());
        MPI_Sendrecv(send_buf.data(), static_cast<int>(send_buf.size()), MPI_DOUBLE, nbr_xp, tag_x_to_plus,
                     recv_buf.data(), static_cast<int>(recv_buf.size()), MPI_DOUBLE, nbr_xp, tag_x_to_minus,
                     cart_workers, MPI_STATUS_IGNORE);
        unpack_rect(ox + owned_w, oy, wR, owned_h, recv_buf);
    }

    // Exchange with upper neighbor: send top owned band, receive into top ghost band
    if (nbr_ym != MPI_PROC_NULL && hT > 0) {
        pack_rect(ox, oy, owned_w, hT, send_buf);
        recv_buf.resize(send_buf.size());
        MPI_Sendrecv(send_buf.data(), static_cast<int>(send_buf.size()), MPI_DOUBLE, nbr_ym, tag_y_to_minus,
                     recv_buf.data(), static_cast<int>(recv_buf.size()), MPI_DOUBLE, nbr_ym, tag_y_to_plus,
                     cart_workers, MPI_STATUS_IGNORE);
        unpack_rect(ox, oy - hT, owned_w, hT, recv_buf);
    }

    // Exchange with lower neighbor: send bottom owned band, receive into bottom ghost band
    if (nbr_yp != MPI_PROC_NULL && hB > 0) {
        pack_rect(ox, oy + owned_h - hB, owned_w, hB, send_buf);
        recv_buf.resize(send_buf.size());
        MPI_Sendrecv(send_buf.data(), static_cast<int>(send_buf.size()), MPI_DOUBLE, nbr_yp, tag_y_to_plus,
                     recv_buf.data(), static_cast<int>(recv_buf.size()), MPI_DOUBLE, nbr_yp, tag_y_to_minus,
                     cart_workers, MPI_STATUS_IGNORE);
        unpack_rect(ox, oy + owned_h, owned_w, hB, recv_buf);
    }
}


static void pack_owned_pheromones(const pheronome& phen,
                                 int owned_w, int owned_h,
                                 int overlap,
                                 std::vector<double>& out_food,
                                 std::vector<double>& out_nest)
{
    out_food.resize(static_cast<std::size_t>(owned_w) * static_cast<std::size_t>(owned_h));
    out_nest.resize(static_cast<std::size_t>(owned_w) * static_cast<std::size_t>(owned_h));

    std::size_t k = 0;
    for (int y = 0; y < owned_h; ++y) {
        for (int x = 0; x < owned_w; ++x) {
            const auto& cell = phen(static_cast<unsigned long>(x + overlap),
                                    static_cast<unsigned long>(y + overlap));
            out_food[k] = cell[0];
            out_nest[k] = cell[1];
            ++k;
        }
    }
}


static void build_root_pheromone_recv_types(std::vector<MPI_Datatype>& recv_types,
                                           int nbp,
                                           int w_nbp,
                                           int Px, int Py,
                                           fractal_land::dim_t global_dim,
                                           int overlap)
{
    recv_types.assign(static_cast<std::size_t>(nbp), MPI_DATATYPE_NULL);

    if (w_nbp <= 0) return;

    const int full_sizes[2] = {
        static_cast<int>(global_dim), // rows
        static_cast<int>(global_dim)  // cols
    };

    for (int world_src = 1; world_src < nbp; ++world_src) {
        const int w_rank = world_src - 1;

        // Layout helper (no data), same partitioning as workers
        fractal_land layout(nullptr, global_dim, w_rank, w_nbp, Px, Py, static_cast<fractal_land::dim_t>(overlap));

        const int sub_sizes[2] = {
            static_cast<int>(layout.owned_height()),      // rows
            static_cast<int>(layout.owned_dimensions())   // cols
        };

        const int starts[2] = {
            static_cast<int>(layout.y_offset()), // start row in global owned coordinates
            static_cast<int>(layout.x_offset())  // start col in global owned coordinates
        };

        MPI_Type_create_subarray(2, full_sizes, sub_sizes, starts,
                                 MPI_ORDER_C, MPI_DOUBLE, &recv_types[world_src]);
        MPI_Type_commit(&recv_types[world_src]);
    }
}

//structure for ant migrations
struct AntMsg {
    int gx;
    int gy;
    int loaded_flag;
    std::uint64_t seed;
};


static void migrate_ants(Population& ants, const fractal_land& land, MPI_Comm cart_workers, int overlap)
{
    if (cart_workers == MPI_COMM_NULL || overlap <= 0) return;

    int nbr_xm = MPI_PROC_NULL, nbr_xp = MPI_PROC_NULL;
    int nbr_ym = MPI_PROC_NULL, nbr_yp = MPI_PROC_NULL;

    // direction 0 = x, direction 1 = y
    MPI_Cart_shift(cart_workers, 0, 1, &nbr_xm, &nbr_xp);
    MPI_Cart_shift(cart_workers, 1, 1, &nbr_ym, &nbr_yp);

    const int dimx    = static_cast<int>(land.dimensions());
    const int dimy    = static_cast<int>(land.height());
    const int owned_w = static_cast<int>(land.owned_dimensions());
    const int owned_h = static_cast<int>(land.owned_height());

    // Actual owned block origin inside local storage (with halos)
    const int ox = static_cast<int>(land.inner_x_begin());
    const int oy = static_cast<int>(land.inner_y_begin());

    if (owned_w <= 0 || owned_h <= 0) return;
    if (ox < 0 || oy < 0 || ox + owned_w > dimx || oy + owned_h > dimy) return;

    const int halo_left   = ox;
    const int halo_right  = dimx - (ox + owned_w);
    const int halo_top    = oy;
    const int halo_bottom = dimy - (oy + owned_h);

    // Effective halo width per side for this rank
    const int wL = std::min(overlap, halo_left);
    const int wR = std::min(overlap, halo_right);
    const int hT = std::min(overlap, halo_top);
    const int hB = std::min(overlap, halo_bottom);

    // 0 = left, 1 = right, 2 = up, 3 = down
    std::array<std::vector<AntMsg>, 4> send_bins;
    std::array<std::vector<AntMsg>, 4> recv_bins;

    // Extract ants currently in halos and prepare outgoing messages
    // Reverse iteration is required because destroy_ant swaps with back.
    for (int i = static_cast<int>(ants.get_size()) - 1; i >= 0; --i) {
        const position_t p = ants.get_position(i);

        int side = -1;
        if (wL > 0 && p.x < ox) side = 0;
        else if (wR > 0 && p.x >= ox + owned_w) side = 1;
        else if (hT > 0 && p.y < oy) side = 2;
        else if (hB > 0 && p.y >= oy + owned_h) side = 3;
        else continue; // still inside owned region

        int dst = MPI_PROC_NULL;
        if (side == 0) dst = nbr_xm;
        if (side == 1) dst = nbr_xp;
        if (side == 2) dst = nbr_ym;
        if (side == 3) dst = nbr_yp;

        if (dst != MPI_PROC_NULL) {
            AntMsg msg{};
            msg.gx = static_cast<int>(land.storage_x_offset()) + p.x;
            msg.gy = static_cast<int>(land.storage_y_offset()) + p.y;
            msg.loaded_flag = ants.is_loaded(i) ? 1 : 0;
            msg.seed = static_cast<std::uint64_t>(ants.get_seed(i));
            send_bins[side].push_back(msg);
        }

        // Remove local ant even if no neighbor exists (global boundary drop policy)
        ants.destroy_ant(i);
    }

    auto exchange_dir = [&](int nbr,
                            int cnt_send_tag, int cnt_recv_tag,
                            int data_send_tag, int data_recv_tag,
                            std::vector<AntMsg>& send_buf,
                            std::vector<AntMsg>& recv_buf)
    {
        if (nbr == MPI_PROC_NULL) return;

        int send_n = static_cast<int>(send_buf.size());
        int recv_n = 0;

        // First exchange message counts
        MPI_Sendrecv(&send_n, 1, MPI_INT, nbr, cnt_send_tag,
                     &recv_n, 1, MPI_INT, nbr, cnt_recv_tag,
                     cart_workers, MPI_STATUS_IGNORE);

        recv_buf.resize(static_cast<std::size_t>(recv_n));

        // Then exchange payload as raw bytes
        MPI_Sendrecv(send_buf.data(), send_n * static_cast<int>(sizeof(AntMsg)), MPI_BYTE, nbr, data_send_tag,
                     recv_buf.data(), recv_n * static_cast<int>(sizeof(AntMsg)), MPI_BYTE, nbr, data_recv_tag,
                     cart_workers, MPI_STATUS_IGNORE);
    };

    // Count tags
    constexpr int C_XM = 700, C_XP = 701, C_YM = 702, C_YP = 703;
    // Data tags
    constexpr int D_XM = 710, D_XP = 711, D_YM = 712, D_YP = 713;

    // Left/right exchanges
    exchange_dir(nbr_xm, C_XM, C_XP, D_XM, D_XP, send_bins[0], recv_bins[0]);
    exchange_dir(nbr_xp, C_XP, C_XM, D_XP, D_XM, send_bins[1], recv_bins[1]);

    // Up/down exchanges
    exchange_dir(nbr_ym, C_YM, C_YP, D_YM, D_YP, send_bins[2], recv_bins[2]);
    exchange_dir(nbr_yp, C_YP, C_YM, D_YP, D_YM, send_bins[3], recv_bins[3]);

    // Insert received ants in local storage coordinates
    for (const auto& bin : recv_bins) {
        for (const auto& msg : bin) {
            const int lx = msg.gx - static_cast<int>(land.storage_x_offset());
            const int ly = msg.gy - static_cast<int>(land.storage_y_offset());

            if (lx < 0 || ly < 0 ||
                lx >= static_cast<int>(land.dimensions()) ||
                ly >= static_cast<int>(land.height())) {
                continue;
            }

            ants.new_ant({lx, ly},
                         (msg.loaded_flag == 1) ? loaded : unloaded,
                         static_cast<std::size_t>(msg.seed));
        }
    }
}



int main(int argc, char** argv)
{
    TimeCounter counter;
    MPI_Init(&argc, &argv);

    int rank = 0, nbp = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nbp);

    // Communicator of workers (all ranks except 0)
    MPI_Comm workers = MPI_COMM_NULL;
    const int color = (rank == 0) ? MPI_UNDEFINED : 1;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &workers);

    constexpr fractal_land::dim_t ln2_dim  = 8UL;
    constexpr unsigned long      nb_seeds  = 2UL;
    constexpr double             deviation = 1.0;
    constexpr int                seed      = 1024;
    constexpr int                overlap   = 1;

    std::size_t seed_pher = 2026; // Seed for random generation (reproducible)

    const int nb_ants = 2000;   // Number of ants per worker process
    const double eps  = 0.8;    // Exploration coefficient
    const double alpha= 0.7;    // Chaos coefficient
    const double beta = 0.999;  // Evaporation coefficient

    // Nest location in GLOBAL coordinates
    position_t pos_nest{256, 256};
    // Food location in GLOBAL coordinates
    position_t pos_food{500, 500};

    std::unique_ptr<fractal_land> full_land;
    unsigned long global_dim_ul = 0;

    // Root will build and keep a full pheromone map (two components) for rendering
    std::vector<double> global_pher_food;
    std::vector<double> global_pher_nest;
    std::vector<Uint32> pher_pixels;
    SDL_Window* sdl_window = nullptr;
    SDL_Renderer* sdl_renderer = nullptr;
    SDL_Texture* sdl_texture = nullptr;

    // Normalize using GLOBAL min/max to avoid per-rank inconsistent scaling
    double global_min_val = 0.0;
    double global_max_val = 0.0;

    if (rank == 0) {
        full_land = std::make_unique<fractal_land>(ln2_dim, nb_seeds, deviation, seed);
        global_dim_ul = static_cast<unsigned long>(full_land->dimensions());

        // Compute global min/max from the full land data
        const std::size_t full_count =
            static_cast<std::size_t>(global_dim_ul) * static_cast<std::size_t>(global_dim_ul);

        auto mm = std::minmax_element(full_land->data(), full_land->data() + full_count);
        global_min_val = *mm.first;
        global_max_val = *mm.second;

        // Allocate root-side full pheromone maps (same size as global land, no ghosts at root)
        global_pher_food.assign(full_count, 0.0);
        global_pher_nest.assign(full_count, 0.0);
    }

    // Everyone needs global dimension and global min/max
    MPI_Bcast(&global_dim_ul,   1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&global_min_val,  1, MPI_DOUBLE,        0, MPI_COMM_WORLD);
    MPI_Bcast(&global_max_val,  1, MPI_DOUBLE,        0, MPI_COMM_WORLD);

    const fractal_land::dim_t global_dim = static_cast<fractal_land::dim_t>(global_dim_ul);

    // Worker grid decomposition (computed once by rank 0 and broadcast)
    int w_nbp = std::max(0, nbp - 1);
    int dims[2] = {0, 0}; // Px, Py for workers
    if (rank == 0 && w_nbp > 0) {
        MPI_Dims_create(w_nbp, 2, dims);
    }
    MPI_Bcast(dims, 2, MPI_INT, 0, MPI_COMM_WORLD);

    const int Px = dims[0];
    const int Py = dims[1];

    // Root-side receive datatypes for pheromone tiles (one per worker world rank)
    std::vector<MPI_Datatype> root_recv_types;
    if (rank == 0) {
        build_root_pheromone_recv_types(root_recv_types, nbp, w_nbp, Px, Py, global_dim, overlap);
    }

    // Worker-side objects (kept outside scopes so they exist in the simulation loop)
    std::unique_ptr<fractal_land> local_land;
    std::unique_ptr<Population>   ants;
    std::unique_ptr<pheronome>    phen;

    // Local storage coordinates (tile + halo), or (-1,-1) if not present in this worker
    position_t pos_nest_local{-1, -1};
    position_t pos_food_local{-1, -1};
    bool has_nest_local = false;
    bool has_food_local = false;

    // Cartesian communicator for halo exchanges among workers
    MPI_Comm cart_workers = MPI_COMM_NULL;

    if (rank == 0) {
        SDL_Init(SDL_INIT_VIDEO);
        const int sdl_dim = static_cast<int>(global_dim_ul);
        sdl_window = SDL_CreateWindow("Ant Pheromone Map",
                                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      sdl_dim, sdl_dim, SDL_WINDOW_SHOWN);
        if (sdl_window != nullptr) {
            sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
            if (sdl_renderer == nullptr) {
                sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_SOFTWARE);
            }
            if (sdl_renderer != nullptr) {
                sdl_texture = SDL_CreateTexture(
                    sdl_renderer,
                    SDL_PIXELFORMAT_ARGB8888,
                    SDL_TEXTUREACCESS_STREAMING,
                    sdl_dim, sdl_dim
                );
                if (sdl_texture != nullptr) {
                    pher_pixels.resize(
                        static_cast<std::size_t>(sdl_dim) * static_cast<std::size_t>(sdl_dim),
                        0xFF000000u
                    );
                }
            }
        }

        // Rank 0: send each worker its subarray (tile + halo)
        if (w_nbp > 0) {
            const int full_sizes[2] = {
                static_cast<int>(global_dim_ul),
                static_cast<int>(global_dim_ul)
            };

            std::vector<MPI_Request>  reqs(nbp - 1, MPI_REQUEST_NULL);
            std::vector<MPI_Datatype> types(nbp, MPI_DATATYPE_NULL);

            for (int world_dst = 1; world_dst < nbp; ++world_dst) {
                const int w_rank = world_dst - 1;

                // fractal_land layout w/o data for sending to workers
                fractal_land layout(nullptr, global_dim, w_rank, w_nbp, Px, Py, static_cast<fractal_land::dim_t>(overlap));

                // MPI subarray: first dim = rows (y/height), second dim = cols (x/width)
                const int sub_sizes[2] = {
                    static_cast<int>(layout.height()),
                    static_cast<int>(layout.dimensions())
                };

                const int starts[2] = {
                    static_cast<int>(layout.storage_y_offset()),
                    static_cast<int>(layout.storage_x_offset())
                };

                MPI_Type_create_subarray(
                    2, full_sizes, sub_sizes, starts,
                    MPI_ORDER_C, MPI_DOUBLE, &types[world_dst]
                );
                MPI_Type_commit(&types[world_dst]);

                MPI_Isend(full_land->data(), 1, types[world_dst],
                          world_dst, land_tag, MPI_COMM_WORLD,
                          &reqs[world_dst - 1]);
            }

            MPI_Waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);

            for (int world_dst = 1; world_dst < nbp; ++world_dst) {
                MPI_Type_free(&types[world_dst]);
            }
        }

    } else {
        // Worker ranks
        int w_rank = 0, w_size = 0;
        MPI_Comm_rank(workers, &w_rank);
        MPI_Comm_size(workers, &w_size);

        // Build a 2D Cartesian communicator for workers
        int cart_dims[2] = {Px, Py};
        int periods[2]   = {0, 0}; // non-periodic boundaries
        MPI_Cart_create(workers, 2, cart_dims, periods, 0, &cart_workers);

        // Make per-rank RNG deterministic but different across ranks
        seed_pher += static_cast<std::size_t>(rank) * 1000003ULL;

        // Allocate local subdomain (tile + halo) and receive it from rank 0
        local_land = std::make_unique<fractal_land>(nullptr, global_dim, w_rank, w_size, Px, Py, static_cast<fractal_land::dim_t>(overlap));

        const std::size_t local_count =
            static_cast<std::size_t>(local_land->dimensions()) *
            static_cast<std::size_t>(local_land->height());

        MPI_Recv(local_land->data(),
                 static_cast<int>(local_count),
                 MPI_DOUBLE,
                 0, land_tag, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        // Normalize consistently using GLOBAL min/max (avoid per-rank scaling artifacts)
        const double delta = global_max_val - global_min_val;
        if (delta > 0.0) {
            for (fractal_land::dim_t y = 0; y < local_land->height(); ++y) {
                for (fractal_land::dim_t x = 0; x < local_land->dimensions(); ++x) {
                    (*local_land)(x, y) = ((*local_land)(x, y) - global_min_val) / delta;
                }
            }
        }

        ants = std::make_unique<Population>(nb_ants);
        ants->set_exploration_coef(eps);

        // Generate ants only in the owned interior region (avoid spawning in ghost cells)
        const int x_min = overlap;
        const int y_min = overlap;
        const int x_max = overlap + static_cast<int>(local_land->owned_dimensions()) - 1;
        const int y_max = overlap + static_cast<int>(local_land->owned_height()) - 1;

        auto gen_ant_x = [&seed_pher, x_min, x_max]() { return rand_int32(x_min, x_max, seed_pher); };
        auto gen_ant_y = [&seed_pher, y_min, y_max]() { return rand_int32(y_min, y_max, seed_pher); };

        for (int i = 0; i < nb_ants; ++i) {
            // This calls push_back internally and increments the vector sizes
            ants->new_ant({gen_ant_x(), gen_ant_y()}, unloaded, seed_pher);
        }

        // Food and nest coordinates in local land verification (global -> local mapping)
        has_nest_local =
            (pos_nest.x >= 0) && (pos_nest.y >= 0) &&
            (static_cast<fractal_land::dim_t>(pos_nest.x) >= local_land->x_offset()) &&
            (static_cast<fractal_land::dim_t>(pos_nest.x) <  local_land->x_offset() + local_land->owned_dimensions()) &&
            (static_cast<fractal_land::dim_t>(pos_nest.y) >= local_land->y_offset()) &&
            (static_cast<fractal_land::dim_t>(pos_nest.y) <  local_land->y_offset() + local_land->owned_height());

        if (has_nest_local) {
            // Convert GLOBAL (x,y) to LOCAL STORAGE (x,y), i.e., including halo
            pos_nest_local = position_t{
                static_cast<int>(static_cast<fractal_land::dim_t>(pos_nest.x) - local_land->storage_x_offset()),
                static_cast<int>(static_cast<fractal_land::dim_t>(pos_nest.y) - local_land->storage_y_offset())
            };
        }

        has_food_local =
            (pos_food.x >= 0) && (pos_food.y >= 0) &&
            (static_cast<fractal_land::dim_t>(pos_food.x) >= local_land->x_offset()) &&
            (static_cast<fractal_land::dim_t>(pos_food.x) <  local_land->x_offset() + local_land->owned_dimensions()) &&
            (static_cast<fractal_land::dim_t>(pos_food.y) >= local_land->y_offset()) &&
            (static_cast<fractal_land::dim_t>(pos_food.y) <  local_land->y_offset() + local_land->owned_height());

        if (has_food_local) {
            // Convert GLOBAL (x,y) to LOCAL STORAGE (x,y), i.e., including halo
            pos_food_local = position_t{
                static_cast<int>(static_cast<fractal_land::dim_t>(pos_food.x) - local_land->storage_x_offset()),
                static_cast<int>(static_cast<fractal_land::dim_t>(pos_food.y) - local_land->storage_y_offset())
            };
        }

        // Pheromones for local land storage (tile + halo)
        phen = std::make_unique<pheronome>(
            static_cast<unsigned long>(local_land->dimensions()),
            static_cast<unsigned long>(local_land->height()),
            pos_food_local, pos_nest_local,
            alpha, beta
        );
    }

    // Counter of the amount of food brought to the nest by ants
    std::size_t food_quantity = 0;

    SDL_Event event;
    bool cont_loop = true;
    std::size_t it = 0;

    // Worker-side temporary buffers for sending pheromones to root
    std::vector<double> send_pher_food;
    std::vector<double> send_pher_nest;

    while (true) {
        // Rank 0 decides whether we keep running
        int keep_running = 1;

        if (rank == 0) {
            counter.start_render();
            ++it;

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    cont_loop = false;
                }
            }

            keep_running = cont_loop ? 1 : 0;
            counter.end_render();
        }

        // Broadcast stop/continue to ALL ranks to avoid workers spinning forever
        MPI_Bcast(&keep_running, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (!keep_running) break;

        if (rank != 0) {
            counter.start_advance();

            // Safe local positions passed to Population::advance
            const position_t pos_nest_local_safe = has_nest_local ? pos_nest_local : position_t{0, 0};
            const position_t pos_food_local_safe = has_food_local ? pos_food_local : position_t{0, 0};

            // Advance local simulation step
            advance_time(*local_land, *phen,
                         pos_nest_local_safe, pos_food_local_safe,
                         food_quantity, *ants);
            //Ants in the Halo migrated
            migrate_ants(*ants, *local_land, cart_workers, overlap);
            // Halo exchange (currently on LAND; keep it here if you later exchange dynamic fields)
            update_ghost_pheromones(*phen, *local_land, cart_workers, overlap);

            // Pack and send pheromones (owned region only) to root for rendering/aggregation
            const int owned_w = static_cast<int>(local_land->owned_dimensions());
            const int owned_h = static_cast<int>(local_land->owned_height());

            pack_owned_pheromones(*phen, owned_w, owned_h, overlap,
                                  send_pher_food, send_pher_nest);

            MPI_Request sreqs[2];
            MPI_Isend(send_pher_food.data(), static_cast<int>(send_pher_food.size()),
                      MPI_DOUBLE, 0, phero_food_tag, MPI_COMM_WORLD, &sreqs[0]);

            MPI_Isend(send_pher_nest.data(), static_cast<int>(send_pher_nest.size()),
                      MPI_DOUBLE, 0, phero_nest_tag, MPI_COMM_WORLD, &sreqs[1]);

            MPI_Waitall(2, sreqs, MPI_STATUSES_IGNORE);

            counter.end_advance();
        } else {
            // Root receives all worker pheromone tiles into the full global arrays
            if (w_nbp > 0) {
                std::vector<MPI_Request> rreqs;
                rreqs.reserve(static_cast<std::size_t>(2 * (nbp - 1)));

                for (int world_src = 1; world_src < nbp; ++world_src) {
                    // Receive component 0 into global_pher_food using a subarray datatype
                    MPI_Request rq0 = MPI_REQUEST_NULL;
                    MPI_Irecv(global_pher_food.data(), 1, root_recv_types[world_src],
                              world_src, phero_food_tag, MPI_COMM_WORLD, &rq0);
                    rreqs.push_back(rq0);

                    // Receive component 1 into global_pher_nest using the same subarray datatype
                    MPI_Request rq1 = MPI_REQUEST_NULL;
                    MPI_Irecv(global_pher_nest.data(), 1, root_recv_types[world_src],
                              world_src, phero_nest_tag, MPI_COMM_WORLD, &rq1);
                    rreqs.push_back(rq1);
                }

                MPI_Waitall(static_cast<int>(rreqs.size()), rreqs.data(), MPI_STATUSES_IGNORE);
            }

            if (sdl_renderer != nullptr && sdl_texture != nullptr && !pher_pixels.empty()) {
                const std::size_t n = pher_pixels.size();
                for (std::size_t i = 0; i < n; ++i) {
                    double rf = global_pher_food[i];
                    if (rf < 0.0) rf = 0.0;
                    if (rf > 1.0) rf = 1.0;

                    double gn = global_pher_nest[i];
                    if (gn < 0.0) gn = 0.0;
                    if (gn > 1.0) gn = 1.0;

                    const Uint32 r = static_cast<Uint32>(rf * 255.0);
                    const Uint32 g = static_cast<Uint32>(gn * 255.0);
                    pher_pixels[i] = 0xFF000000u | (r << 16) | (g << 8);
                }

                const int pitch = static_cast<int>(global_dim_ul) * static_cast<int>(sizeof(Uint32));
                SDL_UpdateTexture(sdl_texture, nullptr, pher_pixels.data(), pitch);
                SDL_RenderClear(sdl_renderer);
                SDL_RenderCopy(sdl_renderer, sdl_texture, nullptr, nullptr);
                SDL_RenderPresent(sdl_renderer);
            }
        }

        // SDL_Delay(10);
    }

    if (rank == 0) {
        if (sdl_texture != nullptr) {
            SDL_DestroyTexture(sdl_texture);
        }
        if (sdl_renderer != nullptr) {
            SDL_DestroyRenderer(sdl_renderer);
        }
        if (sdl_window != nullptr) {
            SDL_DestroyWindow(sdl_window);
        }
        SDL_Quit();

        // Free root receive types
        for (int world_src = 1; world_src < nbp; ++world_src) {
            if (!root_recv_types.empty() && root_recv_types[world_src] != MPI_DATATYPE_NULL) {
                MPI_Type_free(&root_recv_types[world_src]);
            }
        }
    }

    if (cart_workers != MPI_COMM_NULL) {
        MPI_Comm_free(&cart_workers);
    }

    if (workers != MPI_COMM_NULL) {
        MPI_Comm_free(&workers);
    }

    MPI_Finalize();
    return 0;
}
