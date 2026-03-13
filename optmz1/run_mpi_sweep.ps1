$env:PATH += ";C:\Program Files\Microsoft MPI\Bin"
$env:PATH += ";C:\ENSTA\P3\parallele\AntProblem\optmz1\external\SDL2-2.30.0\i686-w64-mingw32\bin"

$RESULTS_DIR = "src/results/mpi_sweep"
if (-not (Test-Path -Path $RESULTS_DIR)) {
    New-Item -ItemType Directory -Path $RESULTS_DIR -Force | Out-Null
}

$env:OMP_NUM_THREADS = 1

$ANTS_LIST = @(5000, 40000, 160000)
$PROCS_LIST = @(1, 2, 4, 6, 8) 

foreach ($ants in $ANTS_LIST) {
    foreach ($p in $PROCS_LIST) {
        $summary_csv = "$RESULTS_DIR/summary_procs${p}_ants${ants}.csv"
        $timing_csv = "$RESULTS_DIR/iter_procs${p}_ants${ants}.csv"

        Write-Host "Running MPI sim: processes=$p, ants=$ants"
        mpiexec -n $p .\src\ant_simu.exe --nb-ants $ants --post-first-food-iterations 10 --timing-csv "$timing_csv" --summary-csv "$summary_csv" --headless
    }
}

Write-Host "Done. Results saved in: optmz1/$RESULTS_DIR"
