clear;clc;

%%%%%%%%%% P2P %%%%%%%%%%

lat_s = [1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864];
lat_sl = {'1', '', '4', '', '16', '', '64', '', '256', '', '1K', '', '4K', '', '16K', '', '64K', '', '256K', '', '1M', '', '4M', '', '16M', '', '64M'};

lat_shmem = [2.765e1 2.789e1 2.789e1 2.790e1 2.786e1 2.794e1 2.794e1 2.797e1 2.804e1 2.810e1 2.826e1 2.870e1 2.964e1 3.149e1 3.438e1 3.671e1 4.065e1 4.860e1 6.490e1 9.723e1 1.616e2 2.899e2 5.472e2 1.061e3 2.088e3 4.139e3 8.240e3];
lat_shmem_ext = [4.374 4.377 4.373 4.373 4.379 4.384 4.396 4.419 4.466 4.496 4.598 4.788 5.174 6.376 7.588 9.672 1.233e1 1.762e1 2.820e1 4.938e1 9.426e1 1.846e2 3.694e2 7.484e2 1.489e3 2.972e3 5.935e3];
lat_mpi = [7.828 7.866 7.867 7.870 7.872 7.872 7.874 7.895 7.986 8.003 8.133 8.575 8.904 9.838 1.104e1 1.313e1 1.579e1 2.132e1 3.197e1 1.084e2 1.798e2 2.655e2 4.459e2 8.182e2 1.554e3 3.017e3 5.738e3];

figure;
loglog(lat_s, lat_shmem, '-d', lat_s, lat_shmem_ext, '-*', lat_s, lat_mpi, '-^', 'LineWidth', 1);
pbaspect([1.4 1 1]);
xlim([1 67108864]);
ylim([4 8.5e3]);
xticks(lat_s);
xticklabels(lat_sl);
xlabel('Message Size (bytes)', 'Interpreter', 'latex', 'fontsize', 14);
ylabel('Latency ($\mu$s)', 'Interpreter', 'latex', 'fontsize', 14);
legend('Standard OpenSHMEM PUT', 'Extended OpenSHMEM PUT', 'Spectrum MPI Send-Receive', 'Location', 'NorthWest', 'Interpreter', 'latex', 'fontsize', 12);
% title('P2P Latency Comparision', 'Interpreter', 'latex', 'fontsize', 14);
set(gcf, 'Position', [400, 300, 700, 350]);
saveas(gcf, 'p2p_lat', 'epsc');

%%%%%%%%%% AtA %%%%%%%%%%

ata_s = [32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152];
ata_sl = {'32', '', '128', '', '512', '', '2K', '', '8K', '', '32K', '', '128K', '', '512K', '', '2M'};

ata_shmem = [2.144e2 2.152e2 2.172e2 2.224e2 2.314e2 2.405e2 2.603e2 3.024e2 3.814e2 5.569e2 9.326e2 1.768e3 3.562e3 7.553e3 1.545e4 3.138e4 6.594e4];
ata_shmem_ext = [8.426e1 8.417e1 8.409e1 8.413e1 8.413e1 8.432e1 8.481e1 1.325e2 2.537e2 4.306e2 7.703e2 1.462e3 2.698e3 5.464e3 1.106e4 2.229e4 4.488e4];
ata_mpi = [7.018e1 7.303e1 7.730e1 9.188e1 8.956e1 9.226e1 1.097e2 1.658e2 2.699e2 4.366e2 7.434e2 1.435e3 3.441e3 6.712e3 1.375e4 2.712e4 5.391e4];

figure;
loglog(ata_s, ata_shmem, '-d', ata_s, ata_shmem_ext, '-*', ata_s, ata_mpi, '-^', 'LineWidth', 1);
pbaspect([1.4 1 1]);
xlim([32 2097152]);
ylim([6e1 6.7e4]);
xticks(ata_s);
xticklabels(ata_sl);
xlabel('Message Size (bytes)', 'Interpreter', 'latex', 'fontsize', 14);
ylabel('Latency ($\mu$s)', 'Interpreter', 'latex', 'fontsize', 14);
legend('Standard OpenSHMEM', 'Extended OpenSHMEM', 'Spectrum MPI', 'Location', 'NorthWest', 'Interpreter', 'latex', 'fontsize', 12);
% title('All-to-All Latency Comparision (48 GPUs)', 'Interpreter', 'latex', 'fontsize', 14);
set(gcf, 'Position', [400, 300, 700, 350]);
saveas(gcf, 'ata_lat', 'epsc');

%%%%%%%%%% GUPs %%%%%%%%%%

gups_s = [2 4 8 16 32 64 128];
gups_sl = {'2', '4', '8', '16', '32', '64', '128'};

gups_shmem = [0.000049520 0.000076314 0.000140202 0.000276374 0.000520058 0.001049583 0.002052301] ./ gups_s;
gups_shmem_ext = [0.000189029 0.000366593 0.000689782 0.001294131 0.002269923 0.004733281 0.009268112] ./ gups_s;
gups_mpi = [0.000083 0.000063 0.000091 0.000143 0.000232 0.000396 0.000682] ./ gups_s;

figure;
loglog(gups_s, gups_shmem, '-d', gups_s, gups_shmem_ext, '-*', gups_s, gups_mpi, '-^', 'LineWidth', 1);
pbaspect([1.4 1 1]);
xlim([2 128]);
ylim([4e-6 1.2e-4]);
xticks(gups_s);
xticklabels(gups_sl);
xlabel('Number of GPUs', 'Interpreter', 'latex', 'fontsize', 14);
ylabel('Per-PE GUPs ($10^9$ Updates/s)', 'Interpreter', 'latex', 'fontsize', 14);
legend('Standard OpenSHMEM', 'Extended OpenSHMEM', 'Spectrum MPI', 'Location', 'SouthWest', 'Interpreter', 'latex', 'fontsize', 12);
% title('GUPs Weak Scaling', 'Interpreter', 'latex', 'fontsize', 14);
set(gcf, 'Position', [400, 300, 700, 350]);
saveas(gcf, 'gups', 'epsc');

%%%%%%%%%% Heat3D %%%%%%%%%%

heat3d_s = [6 12 18 24 30 36 42 48 54 60];
heat3d_sl = {'6', '12', '18', '24', '30', '36', '42', '48', '54', '60'};

heat3d_shmem = [7.96294e7 4.7555e7 2.86485e7 1.9808e7 1.58661e7 1.54175e7 1.44847e7 1.43882e7 1.36448e7 1.30857e7];
heat3d_shmem_ext = [7.79318e7 4.52388e7 2.64546e7 1.79652e7 1.38466e7 1.36734e7 1.26989e7 1.2638e7 1.20264e7 1.13129e7];
heat3d_mpi = [9.53319e7 6.27376e7 4.41716e7 3.48251e7 3.09518e7 3.06267e7 3.02527e7 3.01772e7 2.94628e7 2.93708e7];

heat3d_shmem = (heat3d_shmem(1) ./ heat3d_shmem) ./ (heat3d_s ./ 6) * 100;
heat3d_shmem_ext = (heat3d_shmem_ext(1) ./ heat3d_shmem_ext) ./ (heat3d_s ./ 6) * 100;
heat3d_mpi = (heat3d_mpi(1) ./ heat3d_mpi) ./ (heat3d_s ./ 6) * 100;

figure;
plot(heat3d_s, heat3d_shmem, '-d', heat3d_s, heat3d_shmem_ext, '-*', heat3d_s, heat3d_mpi, '-^', 'LineWidth', 1);
pbaspect([1.4 1 1]);
xlim([6 60]);
ylim([30 115]);
xticks(heat3d_s);
xticklabels(heat3d_sl);
xlabel('Number of GPUs', 'Interpreter', 'latex', 'fontsize', 14);
ylabel('Strong Scaling Efficiency (\%)', 'Interpreter', 'latex', 'fontsize', 14);
legend('Standard OpenSHMEM', 'Extended OpenSHMEM', 'Spectrum MPI', 'Location', 'SouthWest', 'Interpreter', 'latex', 'fontsize', 12);
% title('Heat3D Strong Scaling', 'Interpreter', 'latex', 'fontsize', 14);
set(gcf, 'Position', [400, 300, 700, 350]);
saveas(gcf, 'heat3d', 'epsc');

%%%%%%%%%% FFT %%%%%%%%%%

fft_s = [2 4 8 16 32 64 128];
fft_sl = {'2', '4', '8', '16', '32', '64', '128'};

fft_shmem = [78063.8 72463 69545.2 66190.3 63367.3 38184.5 28685.2];
fft_shmem_ext = [85824.7 65777.7 61156.5 64970.5 62997.6 36228.9 27443.7];
fft_mpi = [96045.6 81263.8 73459.7 74758.3 67924.5 66100.9 39923.2];

fft_shmem = (fft_shmem(1) ./ fft_shmem) ./ (fft_s ./ 2) * 100;
fft_shmem_ext = (fft_shmem_ext(1) ./ fft_shmem_ext) ./ (fft_s ./ 2) * 100;
fft_mpi = (fft_mpi(1) ./ fft_mpi) ./ (fft_s ./ 2) * 100;

figure;
semilogx(fft_s, fft_shmem, '-d', fft_s, fft_shmem_ext, '-*', fft_s, fft_mpi, '-^', 'LineWidth', 1);
pbaspect([1.4 1 1]);
xlim([2 128]);
ylim([0 100]);
xticks(fft_s);
xticklabels(fft_sl);
xlabel('Number of GPUs', 'Interpreter', 'latex', 'fontsize', 14);
ylabel('Strong Scaling Efficiency (\%)', 'Interpreter', 'latex', 'fontsize', 14);
legend('Standard OpenSHMEM', 'Extended OpenSHMEM', 'Spectrum MPI', 'Location', 'NorthEast', 'Interpreter', 'latex', 'fontsize', 12);
% title('FFT Strong Scaling', 'Interpreter', 'latex', 'fontsize', 14);
set(gcf, 'Position', [400, 300, 700, 350]);
saveas(gcf, 'fft', 'epsc');

%%%%%%%%%% Matrix Multiplication %%%%%%%%%%

mm_s = [6 12 18 24 30 36 42 48 54 60];
mm_sl = {'6', '12', '18', '24', '30', '36', '42', '48', '54', '60'};

mm_shmem = [6.45138e7 3.18064e7 2.10111e7 1.56568e7 1.24464e7 1.03501e7 8.87483e6 7.77367e6 6.90967e6 6.2113e6];
mm_shmem_ext = [6.44793e7 3.17459e7 2.09523e7 1.55886e7 1.23876e7 1.02833e7 8.8165e6 7.70455e6 6.84603e6 6.17022e6];
mm_mpi = [6.48072e7 3.207e7 2.12607e7 1.59259e7 1.27245e7 1.06389e7 9.15693e6 8.09611e6 7.25796e6 6.57395e6];

mm_shmem = (mm_shmem(1) ./ mm_shmem) ./ (mm_s ./ 6) * 100;
mm_shmem_ext = (mm_shmem_ext(1) ./ mm_shmem_ext) ./ (mm_s ./ 6) * 100;
mm_mpi = (mm_mpi(1) ./ mm_mpi) ./ (mm_s ./ 6) * 100;

figure;
plot(mm_s, mm_shmem, '-d', mm_s, mm_shmem_ext, '-*', mm_s, mm_mpi, '-^', 'LineWidth', 1);
pbaspect([1.4 1 1]);
xlim([6 60]);
ylim([98 105]);
xticks(mm_s);
xticklabels(mm_sl);
xlabel('Number of GPUs', 'Interpreter', 'latex', 'fontsize', 14);
ylabel('Strong Scaling Efficiency (\%)', 'Interpreter', 'latex', 'fontsize', 14);
legend('Standard OpenSHMEM', 'Extended OpenSHMEM', 'Spectrum MPI', 'Location', 'SouthWest', 'Interpreter', 'latex', 'fontsize', 12);
% title('Matrix Multiplication Strong Scaling', 'Interpreter', 'latex', 'fontsize', 14);
set(gcf, 'Position', [400, 300, 700, 350]);
saveas(gcf, 'mm', 'epsc');
