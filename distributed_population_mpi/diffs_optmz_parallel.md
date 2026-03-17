# `optmz1` vs `parallelisation` — Performance & Compilation Analysis

## Overview

| Aspect | `parallelisation` (original) | `optmz1` (optimized) |
|---|---|---|
| **Parallelism** | OpenMP only (shared memory) | **MPI + OpenMP** (distributed + shared) |
| **Process Model** | Single process, multi-threaded | Multi-process, each multi-threaded |
| **Ant Distribution** | All 5000 ants in one process | `5000 / nbp` ants per MPI rank |
| **Pheromone Sync** | Implicit (shared memory) | Explicit `MPI_Allreduce` with `MPI_MAX` |

---

## 1. Compilation Differences

### CMakeLists.txt

| Feature | `parallelisation` | `optmz1` |
|---|---|---|
| MPI dependency | ❌ Not used | ✅ `find_package(MPI REQUIRED)` + `MPI::MPI_CXX` |
| Threads dependency | ✅ `find_package(Threads)` | ❌ Removed (MPI handles it) |
| OpenMP | ✅ Optional (`ENABLE_OPENMP`) | ✅ Optional (no toggle, just `find_package`) |
| MS-MPI paths | N/A | Hardcoded `MPI_HOME` for Windows |

> [!IMPORTANT]
> `optmz1` **requires** MPI to compile. Without an MPI SDK installed (e.g., MS-MPI on Windows), the build will fail. The original `parallelisation` has no such dependency.

### Makefile

| Feature | `parallelisation` | `optmz1` |
|---|---|---|
| Compiler | `g++` (via [Make_msys2.inc](file:///c:/ENSTA/P3/parallele/AntProblem/optmz1/src/Make_msys2.inc)) | `g++` directly |
| Optimization | **`-O3`** for main target, `-O2` for objects | **`-O2`** uniformly |
| MPI flags | None | `-I"...MPI/Include"` + `-L"...MPI/Lib/x86" -lmsmpi` |
| SDL2 | Via system (MSYS2) | Bundled in `external/SDL2-2.30.0` |

> [!NOTE]
> The **parallelisation** Makefile uses `-O3` for the main compilation target but `-O2` for `.cpp.o` objects — this is likely a **misconfiguration** (the `-O3` flag on the link step won't re-optimize already-compiled objects). The `optmz1` Makefile is cleaner with a uniform `-O2`.

---

## 2. Code Differences

### ant_simu.cpp — The Main Loop

This is the **biggest difference**. The optmz1 version transforms the simulation from single-process to distributed:

```diff
 // parallelisation: single process, all ants
 void advance_time(..., Population& pop)
 
 // optmz1: distributed, each rank has its slice
+void advance_time(..., Population& pop, int rank, int nbp)
```

**Key additions in optmz1:**

| MPI Call | Purpose | Performance Impact |
|---|---|---|
| `MPI_Init` / `MPI_Finalize` | Process lifecycle | Negligible |
| `MPI_Bcast(&cont_loop)` | Sync quit signal | Negligible (1 bool) |
| `phen.synchronize()` → `MPI_Allreduce` | Sync pheromone map | **⚠️ HIGH** — transfers `2 × (dim+2)² × sizeof(double)` bytes every iteration |
| `MPI_Allreduce(&local_delta)` | Sum food deliveries | Negligible (1 `size_t`) |
| `MPI_Gather(local_pos)` | Gather ant positions for display | **Moderate** — `2 × nb_ants_total × sizeof(int)` per iteration |

**Ant distribution:**
```diff
-const int nb_ants = 5000;           // All ants in one process
+const int nb_ants_total = 5000;
+const int nb_ants = nb_ants_total / nbp;  // Split across MPI ranks
```

**Display architecture:**
- Only rank 0 runs SDL and rendering
- A separate `disp_ants` population (full size) is maintained on rank 0
- Ant positions are gathered via `MPI_Gather` every frame

---

### pheronome.hpp — Pheromone Synchronization

Two key changes:

**1. New [synchronize()](file:///c:/ENSTA/P3/parallele/AntProblem/optmz1/include/pheronome.hpp#115-125) method (MPI):**
```cpp
void synchronize() {
    // Uses MPI_MAX: strongest pheromone value wins across all ranks
    MPI_Allreduce(MPI_IN_PLACE, m_buffer_pheronome.data(), 
                  2 * m_buffer_pheronome.size(), 
                  MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
}
```

> [!WARNING]
> This is the **most expensive MPI call** in the simulation. For a 1024×1024 grid, this transfers `2 × 1026² × 8 bytes ≈ 16 MB` of data **every single iteration**. This is the main bottleneck limiting MPI scalability.

**2. Buffer initialization fix in [update()](file:///c:/ENSTA/P3/parallele/AntProblem/parallelisation/include/pheronome.hpp#102-108):**
```diff
 void update() {
     m_map_of_pheronome.swap(m_buffer_pheronome);
     cl_update();
     m_map_of_pheronome[...food...][0] = 1;
     m_map_of_pheronome[...nest...][1] = 1;
+    // Initialize buffer from current map so unvisited cells retain values
+    m_buffer_pheronome = m_map_of_pheronome;
 }
```

> [!NOTE]
> This fix is **not MPI-specific** — it also fixes a bug in the original where unvisited cells would lose pheromone data after swap. This is a correctness improvement that also benefits a single-process run.

---

### rand_generator.hpp — RNG Quality Fix

```diff
 // rand_int32: better distribution using higher bits
-return min_val + seed % ( max_val - min_val + 1 );
+return min_val + (seed >> 16) % ( max_val - min_val + 1 );
 
 // rand_double: proper normalization
-return min_val + std::fmod( seed, ( max_val - min_val + 1 ) );
+return min_val + (double(seed) / 4294967296.0) * ( max_val - min_val );
```

> [!TIP]
> The `>> 16` shift extracts the **higher bits** of the LCG, which have better randomness. The low bits of an LCG have notoriously short periods. The [rand_double](file:///c:/ENSTA/P3/parallele/AntProblem/parallelisation/include/rand_generator.hpp#31-36) fix gives a proper uniform distribution in `[min, max)` instead of integer-like behavior from `fmod`.

---

### population.hpp — Minor Addition

```diff
+void set_position(int index, position_t pos) { positions[index] = pos; }
```
Added to support `MPI_Gather` → display update on rank 0.

---

## 3. Performance Analysis

### Expected Speedup from MPI

| Component | Parallelized? | Scaling |
|---|---|---|
| Ant movement ([advance](file:///c:/ENSTA/P3/parallele/AntProblem/optmz1/src/population.cpp#5-58)) | ✅ Split across ranks | ~Linear with `nbp` |
| Pheromone evaporation | ❌ All ranks do full grid | No speedup |
| Pheromone marking ([mark_pheronome](file:///c:/ENSTA/P3/parallele/AntProblem/parallelisation/include/pheronome.hpp#74-101)) | ✅ Each rank marks only its ants | ~Linear with `nbp` |
| Pheromone sync (`MPI_Allreduce`) | N/A — **communication overhead** | **Increases** with `nbp` |
| Ant position gather | N/A — **communication overhead** | **Increases** with `nbp` |
| Rendering (SDL) | ❌ Rank 0 only | No speedup |

### Bottleneck Analysis

The **dominant cost** per iteration is:

```
T_iteration ≈ T_advance/nbp + T_evaporation + T_MPI_Allreduce + T_MPI_Gather + T_render
```

- **`T_advance/nbp`**: Scales well — each rank processes fewer ants
- **`T_evaporation`**: Constant — all ranks process the full grid (redundant work)
- **`T_MPI_Allreduce`**: ~16 MB per call — **network-bound**, scales poorly
- **`T_MPI_Gather`**: ~40 KB per call — moderate
- **`T_render`**: Constant — rank 0 only

### When MPI Helps vs Hurts

| Scenario | MPI Benefit |
|---|---|
| Many ants (>10K), fast network | ✅ Good speedup |
| Few ants (<1K) | ❌ Communication overhead dominates |
| Large grid (1024²) | ❌ Pheromone sync is very expensive |
| Same machine (shared memory) | ⚠️ Marginal — OpenMP alone may be better |
| Cluster with fast InfiniBand | ✅ Best case for MPI |

---

## 4. Summary

| Category | Change | Impact |
|---|---|---|
| **Compilation** | Adds MPI as **required** dependency | Harder to build, needs MPI SDK |
| **Compilation** | Uniform `-O2` (vs mixed `-O3`/`-O2`) | Cleaner, but potentially slightly slower single-process |
| **Performance** | Ant work split across MPI ranks | ✅ Linear speedup potential |
| **Performance** | `MPI_Allreduce` for pheromones every frame | ⚠️ ~16 MB/iter communication overhead |
| **Correctness** | Buffer copy in [update()](file:///c:/ENSTA/P3/parallele/AntProblem/parallelisation/include/pheronome.hpp#102-108) | ✅ Fixes pheromone data loss bug |
| **Correctness** | Better RNG with bit-shift | ✅ Improved randomness quality |
