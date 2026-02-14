#include "fractal_land.hpp"
#include "rand_generator.hpp"
#include <algorithm>
#include <stdexcept>

void
fractal_land::compute_subgrid(int log_subgrid_dim, int iB, int jB, double deviation, std::size_t seed)
{
    RandomGenerator gen(seed, -deviation, deviation);

    fractal_land& cur_land = *this;
    const unsigned long dim_ss_grid = 1UL << log_subgrid_dim;
    const unsigned long iBeg = iB * dim_ss_grid;
    const unsigned long jBeg = jB * dim_ss_grid;
    const int mid_ind = static_cast<int>(dim_ss_grid / 2UL);
    const int i_mid = static_cast<int>(iBeg) + mid_ind;
    const int j_mid = static_cast<int>(jBeg) + mid_ind;
    const int iEnd = static_cast<int>(iBeg + dim_ss_grid);
    const int jEnd = static_cast<int>(jBeg + dim_ss_grid);

    cur_land(i_mid, jBeg) = 0.5 * (cur_land(iBeg, jBeg) + cur_land(iEnd, jBeg)) + mid_ind * gen(i_mid, jBeg);
    cur_land(iBeg, j_mid) = 0.5 * (cur_land(iBeg, jBeg) + cur_land(iBeg, jEnd)) + mid_ind * gen(iBeg, j_mid);
    cur_land(i_mid, jEnd) = 0.5 * (cur_land(iBeg, jEnd) + cur_land(iEnd, jEnd)) + mid_ind * gen(i_mid, jEnd);
    cur_land(iEnd, j_mid) = 0.5 * (cur_land(iEnd, jBeg) + cur_land(iEnd, jEnd)) + mid_ind * gen(iEnd, j_mid);
    cur_land(i_mid, j_mid) =
        0.25 * (cur_land(i_mid, jBeg) + cur_land(iBeg, j_mid) + cur_land(i_mid, jEnd) + cur_land(iEnd, j_mid)) +
        mid_ind * gen(i_mid, j_mid);
}

fractal_land::fractal_land(const dim_t& ln2_dim, unsigned long nbSeeds, double deviation, int seed)
    : m_dimensions(0),
      m_height(0),
      m_global_dimensions(0),
      nx(0),
      ny(0),
      m_altitude(),
      offset_x(0),
      offset_y(0),
      m_overlap(0),
      storage_offset_x(0),
      storage_offset_y(0),
      inner_x0(0),
      inner_y0(0)
{
    unsigned long dim_ss_grid = 1UL << ln2_dim;
    m_dimensions = nbSeeds * dim_ss_grid + 1UL;
    m_height = m_dimensions;
    m_global_dimensions = m_dimensions;
    m_altitude.assign(m_dimensions * m_height, 0.0);

    RandomGenerator gen(seed, 0.0, dim_ss_grid * deviation);
    fractal_land& cur_land = *this;

    for (dim_t i = 0; i < m_dimensions; i += dim_ss_grid) {
        for (dim_t j = 0; j < m_height; j += dim_ss_grid) {
            cur_land(i, j) = gen(static_cast<int>(i), static_cast<int>(j));
        }
    }

    dim_t ldim = ln2_dim;
    while (ldim > 1) {
        ldim -= 1;
        dim_ss_grid /= 2UL;
        nbSeeds *= 2UL;
        for (unsigned long iB = 0; iB < nbSeeds; ++iB) {
            for (unsigned long jB = 0; jB < nbSeeds; ++jB) {
                compute_subgrid(static_cast<int>(ldim), static_cast<int>(iB), static_cast<int>(jB), deviation, seed);
            }
        }
    }
}

//Constructor for local fractal lands with overlaping
fractal_land::fractal_land(const double* global_altitude, dim_t global_dim,int rank,int nbp,int Px,int Py,dim_t overlap)
    : m_dimensions(0),
      m_height(0),
      m_global_dimensions(global_dim),
      nx(0),
      ny(0),
      m_altitude(),
      offset_x(0),
      offset_y(0),
      m_overlap(overlap),
      storage_offset_x(0),
      storage_offset_y(0),
      inner_x0(0),
      inner_y0(0)
{

    if (nbp <= 0) nbp = 1;
    if (Px <= 0) Px = 1;
    if (Py <= 0) Py = 1;
    if (Px * Py != nbp) {
        Px = nbp;
        Py = 1;
    }
    if (rank < 0) rank = 0;
    if (rank >= nbp) rank = nbp - 1;

    const dim_t bx = m_global_dimensions / static_cast<dim_t>(Px); //Base dimentions according to process
    const dim_t by = m_global_dimensions / static_cast<dim_t>(Py);
    const dim_t rem_x = m_global_dimensions % static_cast<dim_t>(Px);
    const dim_t rem_y = m_global_dimensions % static_cast<dim_t>(Py);

    const int rx = rank % Px;
    const int ry = rank / Px;

    nx = bx + (static_cast<dim_t>(rx) < rem_x ? 1UL : 0UL); //dimentions according to process
    ny = by + (static_cast<dim_t>(ry) < rem_y ? 1UL : 0UL);

    offset_x = static_cast<dim_t>(rx) * bx + std::min(static_cast<dim_t>(rx), rem_x);//offset of coordinates in real map
    offset_y = static_cast<dim_t>(ry) * by + std::min(static_cast<dim_t>(ry), rem_y);

    const dim_t halo_left = std::min(m_overlap, offset_x);   //overlaping
    const dim_t halo_top = std::min(m_overlap, offset_y);
    const dim_t halo_right = std::min(m_overlap, m_global_dimensions - (offset_x + nx));
    const dim_t halo_bottom = std::min(m_overlap, m_global_dimensions - (offset_y + ny));

    storage_offset_x = offset_x - halo_left;
    storage_offset_y = offset_y - halo_top;

    m_dimensions = nx + halo_left + halo_right;
    m_height = ny + halo_top + halo_bottom;
    m_altitude.assign(m_dimensions * m_height, 0.0);

    inner_x0 = halo_left;
    inner_y0 = halo_top;

    // If global_altitude is null, this constructor only builds the local layout/buffer.
    if (global_altitude != nullptr) {
        for (dim_t j = 0; j < m_height; ++j) {
            const dim_t jg = storage_offset_y + j;
            const dim_t g_row = jg * m_global_dimensions;
            const dim_t l_row = j * m_dimensions;
            for (dim_t i = 0; i < m_dimensions; ++i) {
                const dim_t ig = storage_offset_x + i;
                m_altitude[l_row + i] = global_altitude[g_row + ig];
            }
        }
    }
}
