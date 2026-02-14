#ifndef _PHERONOME_HPP_
#define _PHERONOME_HPP_

#include <algorithm>
#include <array>
#include <cassert>
#include <vector>
#include "basic_types.hpp"

/**
 * @brief Pheromone map
 * @details Manages a pheromone map and its updates (including evaporation).
 *
 * IMPORTANT CONVENTION (matches MPI subarray usage and typical row-major layout):
 *   - position_t.x = column index (horizontal, "x")
 *   - position_t.y = row index (vertical, "y")
 *   - Linear index = y * stride + x
 *   - stride = dimx (number of columns)
 *
 * ALSO IMPORTANT:
 *   dimx/dimy AND (x,y) coordinates already include ghost/phantom cells.
 *   Border (ghost) cells are at:
 *     x = 0 or x = dimx-1
 *     y = 0 or y = dimy-1
 */
class pheronome {
public:
    using size_t      = unsigned long;
    using pheronome_t = std::array<double, 2>;

    /**
     * @brief Build an initial pheromone map
     * @details The pheromone map is initialized to zero (neutral),
     *          except borders (ghost cells) which are set to -1 to be undesirable.
     *
     * @param dimx Number of columns INCLUDING ghost cells
     * @param dimy Number of rows INCLUDING ghost cells
     * @param pos_food Food position in local storage coordinates (may be invalid if not present)
     * @param pos_nest Nest position in local storage coordinates (may be invalid if not present)
     * @param alpha Noise/chaos parameter
     * @param beta Evaporation factor
     */
    pheronome(size_t dimx, size_t dimy,
              const position_t& pos_food, const position_t& pos_nest,
              double alpha = 0.7, double beta = 0.9999)
        : m_dimx(dimx),
          m_dimy(dimy),
          m_stride(dimx),
          m_alpha(alpha),
          m_beta(beta),
          m_map_of_pheronome(m_dimx * m_dimy, {{0., 0.}}),
          m_buffer_pheronome(),
          m_pos_nest(pos_nest),
          m_pos_food(pos_food),
          m_has_nest(false),
          m_has_food(false)
    {
        // With neighbor accesses (x±1,y±1), we need at least a 3x3 grid (ghost + one interior layer)
        assert(m_dimx >= 3 && m_dimy >= 3);

        // Mark borders (ghost cells) as undesirable
        cl_update();

        // Decide if nest/food are valid in this local storage grid
        m_has_food = is_valid_inside(m_pos_food);
        m_has_nest = is_valid_inside(m_pos_nest);

        // Mark sources only if they exist in this rank
        if (m_has_food) m_map_of_pheronome[index(m_pos_food)][0] = 1.;
        if (m_has_nest) m_map_of_pheronome[index(m_pos_nest)][1] = 1.;

        // Buffer starts as current state
        m_buffer_pheronome = m_map_of_pheronome;
    }

    pheronome(const pheronome&) = delete;
    pheronome(pheronome&&)      = delete;
    ~pheronome()                = default;

    // Access by (x,y) = (column,row)
    pheronome_t& operator()(size_t x, size_t y) {
        return m_map_of_pheronome[y * m_stride + x];
    }
    const pheronome_t& operator()(size_t x, size_t y) const {
        return m_map_of_pheronome[y * m_stride + x];
    }

    pheronome_t& operator[](const position_t& pos) {
        return m_map_of_pheronome[index(pos)];
    }
    const pheronome_t& operator[](const position_t& pos) const {
        return m_map_of_pheronome[index(pos)];
    }

    void do_evaporation() {
        // Do not touch borders (ghost cells)
        for (std::size_t y = 1; y < m_dimy - 1; ++y) {
            for (std::size_t x = 1; x < m_dimx - 1; ++x) {
                m_buffer_pheronome[y * m_stride + x][0] *= m_beta;
                m_buffer_pheronome[y * m_stride + x][1] *= m_beta;
            }
        }
    }

    void mark_pheronome(const position_t& pos) {
        const std::size_t x = static_cast<std::size_t>(pos.x);
        const std::size_t y = static_cast<std::size_t>(pos.y);

        // Ignore invalid/border coordinates to keep simulation robust.
        if (x == 0 || y == 0 || x + 1 >= m_dimx || y + 1 >= m_dimy) {
            return;
        }

        pheronome& phen = *this;

        const pheronome_t& left_cell   = phen(x - 1, y);
        const pheronome_t& right_cell  = phen(x + 1, y);
        const pheronome_t& upper_cell  = phen(x, y - 1);
        const pheronome_t& bottom_cell = phen(x, y + 1);

        const double v1_left   = std::max(left_cell[0],   0.);
        const double v2_left   = std::max(left_cell[1],   0.);
        const double v1_right  = std::max(right_cell[0],  0.);
        const double v2_right  = std::max(right_cell[1],  0.);
        const double v1_upper  = std::max(upper_cell[0],  0.);
        const double v2_upper  = std::max(upper_cell[1],  0.);
        const double v1_bottom = std::max(bottom_cell[0], 0.);
        const double v2_bottom = std::max(bottom_cell[1], 0.);

        m_buffer_pheronome[y * m_stride + x][0] =
            m_alpha * std::max({v1_left, v1_right, v1_upper, v1_bottom}) +
            (1. - m_alpha) * 0.25 * (v1_left + v1_right + v1_upper + v1_bottom);

        m_buffer_pheronome[y * m_stride + x][1] =
            m_alpha * std::max({v2_left, v2_right, v2_upper, v2_bottom}) +
            (1. - m_alpha) * 0.25 * (v2_left + v2_right + v2_upper + v2_bottom);
    }

    void update() {
        // Swap: map becomes the newly computed buffer
        m_map_of_pheronome.swap(m_buffer_pheronome);

        // Reimpose ghost boundaries
        cl_update();

        // Reimpose sources (only if present in this rank)
        if (m_has_food) m_map_of_pheronome[index(m_pos_food)][0] = 1.;
        if (m_has_nest) m_map_of_pheronome[index(m_pos_nest)][1] = 1.;
    }

private:
    // Returns true if pos is a valid coordinate inside the storage grid (including borders).
    bool is_valid_storage(const position_t& pos) const {
        return (pos.x >= 0) && (pos.y >= 0) &&
               (static_cast<unsigned long>(pos.x) < m_dimx) &&
               (static_cast<unsigned long>(pos.y) < m_dimy);
    }

    // Returns true if pos is valid AND not on border (interior cell).
    bool is_valid_inside(const position_t& pos) const {
        if (!is_valid_storage(pos)) return false;
        return (pos.x > 0) && (pos.y > 0) &&
               (static_cast<unsigned long>(pos.x) + 1 < m_dimx) &&
               (static_cast<unsigned long>(pos.y) + 1 < m_dimy);
    }

    size_t index(const position_t& pos) const {
        // Caller must ensure pos is valid
        return static_cast<size_t>(pos.y) * m_stride + static_cast<size_t>(pos.x);
    }

    /**
     * @brief Update boundary conditions on ghost cells
     * @details For now, all borders are set to -1 to ensure ants avoid these cells.
     */
    void cl_update() {
        // Top & bottom rows (y = 0 and y = dimy-1)
        for (std::size_t x = 0; x < m_dimx; ++x) {
            m_map_of_pheronome[0 * m_stride + x]              = {{-1., -1.}};
            m_map_of_pheronome[(m_dimy - 1) * m_stride + x]   = {{-1., -1.}};
        }
        // Left & right columns (x = 0 and x = dimx-1)
        for (std::size_t y = 0; y < m_dimy; ++y) {
            m_map_of_pheronome[y * m_stride + 0]              = {{-1., -1.}};
            m_map_of_pheronome[y * m_stride + (m_dimx - 1)]   = {{-1., -1.}};
        }
    }

    unsigned long m_dimx, m_dimy, m_stride;
    double m_alpha, m_beta;
    std::vector<pheronome_t> m_map_of_pheronome, m_buffer_pheronome;

    // Stored as local storage coordinates (may be invalid if not present in this rank)
    position_t m_pos_nest, m_pos_food;
    bool m_has_nest, m_has_food;
};

#endif
