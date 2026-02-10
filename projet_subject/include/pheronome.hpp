#ifndef _PHERONOME_HPP_
#define _PHERONOME_HPP_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>
#include "basic_types.hpp"


class pheronome {
public:
    using size_t = unsigned long;

    /**
     * @param dim       taille du domaine réel : dim x dim
     * @param pos_src   position de la source (food OU nest)
     * @param alpha     poids du max vs moyenne (0..1)
     * @param beta      facteur d'évaporation (~1)
     * @param src_value valeur imposée à la source
     */
    pheronome(size_t dim,
              const position_t& pos_src,
              double alpha = 0.7,
              double beta  = 0.9999,
              double src_value = 1.0)
        : m_dim(dim),
          m_stride(dim + 2),
          m_alpha(alpha),
          m_beta(beta),
          m_map(m_stride * m_stride, 0.0),
          m_buf(m_stride * m_stride, 0.0),
          m_pos_src(pos_src),
          m_src_value(src_value)
    {
        // Source fixe
        m_map[index(pos_src)] = m_src_value;

        // Bords (ghost cells) à -1
        cl_update();

        // Buffer initial = map
        m_buf = m_map;
    }

    pheronome(const pheronome&) = delete;
    pheronome(pheronome&&)      = delete;
    ~pheronome()                = default;


    double& operator()(size_t i, size_t j) {
        return m_map[idx(i + 1, j + 1)];
    }
    double operator()(size_t i, size_t j) const {
        return m_map[idx(i + 1, j + 1)];
    }

    double& operator[](const position_t& pos) {
        return m_map[index(pos)];
    }
    double operator[](const position_t& pos) const {
        return m_map[index(pos)];
    }

    inline size_t linear_index(const position_t& pos) const {
        // pos.x,pos.y dans [0..dim-1] -> shift +1
        return idx(static_cast<size_t>(pos.x) + 1, static_cast<size_t>(pos.y) + 1);
    }

    void reset_buffer_from_map() {
        m_buf = m_map; // memcpy-like, contigu
    }

    void do_evaporation() {
        double* __restrict b = m_buf.data();
        for (size_t ii = 1; ii <= m_dim; ++ii) {
            const size_t base = ii * m_stride;
            #pragma omp simd
            for (size_t jj = 1; jj <= m_dim; ++jj) {
                const size_t k = base + jj;
                b[k] *= m_beta;
            }
        }
    }

    
    void apply_marks_from_touched(std::vector<size_t>& touched) {
        if (touched.empty()) return;

        // Déduplication (in-place)
        std::sort(touched.begin(), touched.end());
        touched.erase(std::unique(touched.begin(), touched.end()), touched.end());

        // Mise à jour du buffer (parallélisable si tu veux mettre un omp parallel for au-dessus)
        // On lit m_map (read-only) et on écrit m_buf.
        #pragma omp parallel for if(touched.size() > 1024)
        for (std::ptrdiff_t t = 0; t < static_cast<std::ptrdiff_t>(touched.size()); ++t) {
            mark_index_to_buffer(static_cast<size_t>(touched[t]));
        }
    }

    void update() {
        m_map.swap(m_buf);
        cl_update();
        m_map[index(m_pos_src)] = m_src_value; // ré-imposer la source
    }

private:

    inline size_t idx(size_t ii, size_t jj) const {
        // ii,jj dans [0..dim+1] (ghost cells incluses)
        return ii * m_stride + jj;
    }

    inline size_t index(const position_t& pos) const {
        return idx(static_cast<size_t>(pos.x) + 1, static_cast<size_t>(pos.y) + 1);
    }

    inline void mark_index_to_buffer(size_t c) {
        // c est un index interne (avec ghost), supposé dans l'intérieur [1..dim]x[1..dim]
        const double* __restrict in = m_map.data();
        double* __restrict out      = m_buf.data();

        const size_t L = c - m_stride;
        const size_t R = c + m_stride;
        const size_t U = c - 1;
        const size_t D = c + 1;

        const double l = std::max(in[L], 0.0);
        const double r = std::max(in[R], 0.0);
        const double u = std::max(in[U], 0.0);
        const double d = std::max(in[D], 0.0);

        const double mx = std::max(std::max(l, r), std::max(u, d));
        const double sm = (l + r + u + d);

        out[c] = m_alpha * mx + (1.0 - m_alpha) * 0.25 * sm;
    }

    void cl_update() {
        for (size_t j = 0; j < m_stride; ++j) {
            m_map[idx(0, j)]         = -1.0;
            m_map[idx(m_dim + 1, j)] = -1.0;
            m_map[idx(j, 0)]         = -1.0;
            m_map[idx(j, m_dim + 1)] = -1.0;
        }
    }

private:
    size_t m_dim, m_stride;
    double m_alpha, m_beta;

    std::vector<double> m_map;
    std::vector<double> m_buf;

    position_t m_pos_src;
    double     m_src_value;
};

#endif
