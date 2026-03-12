# Activités de la première parallelisation (MPI Method 1)

## Objectif
Implémenter la méthode 1 de parallélisation à mémoire distribuée:
- Chaque processus contient l'environnement entier.
- Chaque processus gère une partie des fourmis.
- Mise à jour des phéromones synchronisée par réduction (MAX).

## Checklist
- [x] Initialization de l'environnement MPI (`MPI_Init`, `MPI_Finalize`)
- [x] Distribution des fourmis (partage de `nb_ants` entre les processus)
- [x] Modification de la boucle principale:
    - [x] `advance_time` sur les fourmis locales
    - [x] Synchronisation des phéromones (`MPI_Allreduce` avec `MPI_MAX`)
    - [x] Gestion de l'évaporation (peut être faite localement après réduction, ou parallélisée si on divise la carte - ici on suppose duplication complète pour simplifier selon Method 1 "chaque processus contienne l'environnement en entier")
- [x] Gestion de l'affichage (Rank 0 uniquement)
- [x] Compilation avec `mpic++` (Configurée mais échec outil non trouvé)
- [ ] Tests et Mesures de performance