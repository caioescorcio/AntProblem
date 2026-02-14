#ifndef _FRACTAL_LAND_HPP_
# define _FRACTAL_LAND_HPP_
// Génération d'un fractal pour le coût énergie ( altitude ) de déplacement d'une fourmie
// L'algorithme prend plusieurs paramètres :
// 0. Taille : Nombre de "cases" par direction ( pair d'entier )
// 1. Nombre de graînes : Nombre de points initiaux par direction dont on détermine l'altitude à l'initialisation de l'agorithme
// 2. Déviation : degré de variation de l'altitude en fonction de la distance
// 3. Graîne aléatoire : détermine le paysage à retrouver
# include <vector>
# include <utility>

/**
 * @brief Génère un paysage fractal à l'aide d'un algorithme pseudo-aléatoire 
 * @details 
 *     Génère de façon récursive un paysage fractal à l'aide de algorithme pseudo-aléatoire :
 *        1. Créée une grille de taille \f$nbSeeds*2^{log\_size}+1\f$ cases par directions
 *        2. Génère une altitude pour les cases ayant des indices i et j multiples de 
 *           \f$2^{log\_size}\f$ de telle sorte que le gradient d'altitude entre deux points ne dépasse pas la valeur deviation
 *           On considère alors les sous-grilles ayant pour indices mimimals : \f$Ib*2^{log\_size}\f$, \f$Jb*2^{log\_size}\f$ avec
 *           Ib et Jb compris entre 0 et nbSeeds ( compris ) et de tailles \f$2^{log\_size}\f$.
 *           On note n=log_size le niveau des sous--grilles initiales.
 *        3. Pour chaque sous--grille, on génère l'altitude des points d'indices locaux multiples de \f$2^{n-1}\f$ ormi pour les
 *           coins de la grille en respectant le gradient de déviation.
 *        4. Puis on considère les sous--grilles de niveau n-1 auxquelles on reapplique l'algorithme à partir de 3 et on s'arrête dès que
 *           le niveau de la grille atteint zéro.
 * @param log_size Le logarithme base 2 de la dimension de chaque sous-grille initiale
 * @param nbSeeds  Le nombre de sous-grilles initiales par direction
 * @param deviation La valeur maximale du gradient entre deux altitudes.
 * @param seed Graîne de génération aléatoire
 * @return Un tableau contenant la carte des altitudes en fonctions des indices i et j.
 */
class fractal_land
{
public:
    using container=std::vector<double>;
    using dim_t=unsigned long;

    fractal_land(const double* global_altitude,dim_t global_dim, int rank,int nbp,int Px, int Py,dim_t overlap);

    fractal_land( const dim_t& ln2_dim, unsigned long nbSeeds, double deviation, int seed );

    fractal_land( const fractal_land& ) = delete;

    fractal_land( fractal_land&& land ) = default;

    ~fractal_land() = default;

    const double& operator () ( unsigned long i, unsigned long j ) const {
        return m_altitude[i+j*m_dimensions];
    }
    double& operator () ( unsigned long i, unsigned long j ) {
        return m_altitude[i+j*m_dimensions];
    }
    dim_t dimensions() const { return m_dimensions; }
    dim_t height() const { return m_height; }
    dim_t owned_dimensions() const { return nx; }
    dim_t owned_height() const { return ny; }
    dim_t global_dimensions() const { return m_global_dimensions; }
    dim_t x_offset() const { return offset_x; }
    dim_t y_offset() const { return offset_y; }
    dim_t storage_x_offset() const { return storage_offset_x; }
    dim_t storage_y_offset() const { return storage_offset_y; }
    dim_t overlap() const { return m_overlap; }
    dim_t inner_x_begin() const { return inner_x0; }
    dim_t inner_y_begin() const { return inner_y0; }
    dim_t inner_x_end() const { return inner_x0 + nx; }
    dim_t inner_y_end() const { return inner_y0 + ny; }
    double* data() { return m_altitude.data(); }
    const double* data() const { return m_altitude.data(); }

private:
    void compute_subgrid( int log_subgrid_dim, int iB, int jB, double deviation, std::size_t seed );
    dim_t m_dimensions;
    dim_t m_height;
    dim_t m_global_dimensions;
    dim_t nx;
    dim_t ny;
    container m_altitude;
    dim_t offset_x;
    dim_t offset_y;
    dim_t m_overlap;
    dim_t storage_offset_x;
    dim_t storage_offset_y;
    dim_t inner_x0;
    dim_t inner_y0;
};
#endif
