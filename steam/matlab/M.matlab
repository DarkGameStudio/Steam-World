function steam_storage_lab()
%STEAM_STORAGE_LAB  Design lab for game asset storage management.
%
%   Models and optimizes the storage algorithms that a game backend runs:
%     1. Content-addressed dedup over a simulated asset library
%     2. Optimal chunk size via a cost model + sweep
%     3. Delta patch prediction across a version chain
%     4. Steam Cloud conflict modelling
%     5. Streaming prefetch as a 0/1 knapsack
%     6. Export tuned constants to a C++ header
%
%   No toolboxes required. Uses base MATLAB plus bundled Java for SHA-1
%   (falls back to a pure-MATLAB FNV-1a if Java is unavailable).

    rng(42, 'twister');
    fprintf('\nSTEAM STORAGE LAB\n');
    fprintf('Reproducible run, seed = 42\n');

    tuned = struct();

    % ==================================================================
    print_section('1. CONTENT-ADDRESSED STORE — dedup over a library');
    % ==================================================================
    [assets, blockPool] = make_library(120, 48, 96*1024, 1024*1024);
    totalRaw = sum([assets.size]);

    fprintf('  Simulated library: %d assets, %.1f MB raw\n', ...
            numel(assets), totalRaw / 1e6);

    % Chunk at 256 KB and hash every chunk.
    S0 = 256 * 1024;
    store = containers.Map('KeyType', 'char', 'ValueType', 'double');
    bytesStored = 0;
    bytesSaved  = 0;
    chunkCount  = 0;

    for a = 1:numel(assets)
        data = assets(a).data;
        for off = 1:S0:numel(data)
            hi = min(off + S0 - 1, numel(data));
            c  = data(off:hi);
            h  = hash_bytes(c);
            chunkCount = chunkCount + 1;
            if isKey(store, h)
                bytesSaved = bytesSaved + numel(c);
            else
                store(h) = numel(c);
                bytesStored = bytesStored + numel(c);
            end
        end
    end

    fprintf('  Chunks total      : %d\n', chunkCount);
    fprintf('  Unique blobs      : %d\n', store.Count);
    fprintf('  Stored bytes      : %.1f MB\n', bytesStored / 1e6);
    fprintf('  Saved by dedup    : %.1f MB (%.1f%%)\n', ...
            bytesSaved / 1e6, 100 * bytesSaved / totalRaw);
    fprintf('  Dedup ratio       : %.2fx\n', totalRaw / bytesStored);

    tuned.dedupRatio = totalRaw / bytesStored;

    % ==================================================================
    print_section('2. OPTIMAL CHUNK SIZE — cost model + sweep');
    % ==================================================================
    % Build a 32 MB file, apply realistic edits, and sweep chunk size.
    N       = 32 * 1024 * 1024;
    base    = uint8(randi([0 255], 1, N));
    edited  = base;
    nEdits  = 400;
    for k = 1:nEdits
        p  = randi([1 N]);
        L  = randi([16 8192]);
        hi = min(p + L - 1, N);
        edited(p:hi) = uint8(randi([0 255], 1, hi - p + 1));
    end

    chunkSizes = 2 .^ (12:2:22);          % 4 KB .. 4 MB
    mEntries   = 48;                       % bytes per manifest entry
    bwCost     = 1.0;                      % cost per byte shipped
    metaCost   = 0.02;                     % cost per manifest entry
    reqCost    = 0.001;                    % cost per HTTP range request

    results = struct('size', {}, 'deltaBytes', {}, 'deltaChunks', {}, ...
                     'metaBytes', {}, 'totalCost', {});

    fprintf('  %10s  %12s  %10s  %12s\n', ...
            'chunk', 'delta MB', 'chunks', 'cost');
    fprintf('  %s\n', repmat('-', 1, 52));

    for s = chunkSizes
        d = chunk_diff(base, edited, s);
        nChunks   = ceil(N / s);
        metaBytes = nChunks * mEntries;
        cost = bwCost   * d.bytesChanged ...
             + metaCost * metaBytes ...
             + reqCost  * d.chunksChanged;
        results(end+1) = struct('size', s, 'deltaBytes', d.bytesChanged, ...
                                'deltaChunks', d.chunksChanged, ...
                                'metaBytes', metaBytes, 'totalCost', cost);
        fprintf('  %10s  %12.3f  %10d  %12.4f\n', ...
                human_bytes(s), d.bytesChanged / 1e6, ...
                d.chunksChanged, cost);
    end

    [~, best] = min([results.totalCost]);
    Sstar = results(best).size;
    tuned.chunkSizeBytes = Sstar;
    fprintf('\n  Optimal chunk size : %s\n', human_bytes(Sstar));
    fprintf('  Full-file download would be %.1f MB; delta is %.1f MB (%.2f%%)\n', ...
            N / 1e6, results(best).deltaBytes / 1e6, ...
            100 * results(best).deltaBytes / N);

    figure('Name', 'Chunk-size sweep', 'Color', 'w');
    subplot(1,2,1);
    plot([results.size], [results.deltaBytes] / 1e6, '-o', 'LineWidth', 1.6);
    set(gca, 'XScale', 'log'); grid on;
    xlabel('chunk size (bytes)'); ylabel('delta (MB)');
    title('Delta size vs chunk size');

    subplot(1,2,2);
    plot([results.size], [results.totalCost], '-o', 'LineWidth', 1.6);
    set(gca, 'XScale', 'log'); grid on; hold on;
    plot(Sstar, results(best).totalCost, 'rp', 'MarkerSize', 14, ...
         'MarkerFaceColor', 'r');
    xlabel('chunk size (bytes)'); ylabel('weighted cost');
    title('Total cost — optimal marked');

    % ==================================================================
    print_section('3. DELTA PATCHING — version chain prediction');
    % ==================================================================
    versions = 8;
    v = uint8(randi([0 255], 1, 16 * 1024 * 1024));   % 16 MB starting build
    totalShipped = 0;
    fprintf('  %6s  %12s  %10s  %10s\n', ...
            'ver', 'build MB', 'delta MB', 'delta %');
    fprintf('  %s\n', repmat('-', 1, 44));

    for k = 2:versions
        prev = v;
        nEd  = randi([50 250]);
        for e = 1:nEd
            p  = randi([1 numel(v)]);
            L  = randi([32 4096]);
            hi = min(p + L - 1, numel(v));
            v(p:hi) = uint8(randi([0 255], 1, hi - p + 1));
        end
        d = chunk_diff(prev, v, Sstar);
        totalShipped = totalShipped + d.bytesChanged;
        fprintf('  %6d  %12.1f  %10.3f  %9.3f%%\n', ...
                k, numel(v) / 1e6, d.bytesChanged / 1e6, ...
                100 * d.bytesChanged / numel(v));
    end

    fullRedownload = (versions - 1) * numel(v);
    fprintf('\n  Total shipped with deltas : %.1f MB\n', totalShipped / 1e6);
    fprintf('  Full redownloads would be : %.1f MB\n', fullRedownload / 1e6);
    fprintf('  Bandwidth saved           : %.1fx\n', fullRedownload / totalShipped);
    tuned.deltaRatio = totalShipped / fullRedownload;

    % ==================================================================
    print_section('4. STEAM CLOUD — conflict rate vs sync interval');
    % ==================================================================
    % Model two clients editing the same save. Long sync intervals raise
    % the chance both diverge before either flushes.
    intervals = [5 15 30 60 120 300 600];   % seconds
    sessions  = 2000;                        % games played
    pEdit     = 1 / 40;                      % chance of an edit per second
    rate      = zeros(size(intervals));

    for ii = 1:numel(intervals)
        T = intervals(ii);
        conflicts = 0;
        for g = 1:sessions
            % Each client makes ~Poisson(pEdit * T) edits before syncing.
            eA = poissrnd(pEdit * T);
            eB = poissrnd(pEdit * T);
            if eA > 0 && eB > 0
                conflicts = conflicts + 1;
            end
        end
        rate(ii) = conflicts / sessions;
    end

    fprintf('  %10s  %14s\n', 'interval s', 'conflict rate');
    fprintf('  %s\n', repmat('-', 1, 28));
    for ii = 1:numel(intervals)
        fprintf('  %10d  %13.2f%%\n', intervals(ii), 100 * rate(ii));
    end

    % Pick the interval whose conflict rate is below 2 percent.
    ok = find(rate < 0.02, 1, 'first');
    if isempty(ok), ok = numel(intervals); end
    tuned.cloudSyncSec = intervals(ok);
    fprintf('\n  Chosen sync interval: %d s (conflict rate %.2f%%)\n', ...
            tuned.cloudSyncSec, 100 * rate(ok));

    figure('Name', 'Cloud conflict rate', 'Color', 'w');
    semilogx(intervals, 100 * rate, '-o', 'LineWidth', 1.6); grid on;
    xlabel('sync interval (s)'); ylabel('conflict rate (%)');
    title('Steam Cloud conflict vs sync interval'); hold on;
    yline(2, 'r--', 'target 2%');
    plot(tuned.cloudSyncSec, 100 * rate(ok), 'rp', ...
         'MarkerSize', 14, 'MarkerFaceColor', 'r');

    % ==================================================================
    print_section('5. STREAMING PREFETCH — knapsack under a budget');
    % ==================================================================
    % Given a bandwidth budget and a set of assets, pick the subset that
    % maximizes loading priority without exceeding the window.
    nAssets   = 40;
    sizes     = randi([64 4096], 1, nAssets);    % KB
    priority  = zeros(1, nAssets);
    for i = 1:nAssets
        % Priority correlates with size and with proximity to the player.
        dist = rand();
        priority(i) = round(100 * (1 - dist) + 20 * rand());
    end

    budgetKB = 8000;                              % 8 MB stream window
    [pick, totalVal, totalSz] = knapsack01(sizes, priority, budgetKB);

    fprintf('  Assets considered  : %d\n', nAssets);
    fprintf('  Budget             : %d KB\n', budgetKB);
    fprintf('  Assets selected    : %d\n', numel(pick));
    fprintf('  Bytes loaded       : %d KB (%.1f%% of budget)\n', ...
            totalSz, 100 * totalSz / budgetKB);
    fprintf('  Priority captured  : %d of %d (%.1f%%)\n', ...
            totalVal, sum(priority), 100 * totalVal / sum(priority));

    tuned.prefetchBudgetKB = budgetKB;
    tuned.prefetchFillPct  = 100 * totalSz / budgetKB;

    figure('Name', 'Prefetch selection', 'Color', 'w');
    bar([sizes(:) priority(:)]);
    legend({'size (KB)', 'priority'}, 'Location', 'northoutside', ...
           'Orientation', 'horizontal');
    title('Asset size vs priority — knapsack picks winners');
    grid on;

    % ==================================================================
    print_section('6. EXPORT — tuned constants to a C++ header');
    % ==================================================================
    outFile = 'storage_tuned.h';
    fid = fopen(outFile, 'w');
    if fid < 0
        warning('Could not open %s for writing.', outFile);
    else
        fprintf(fid, '// Auto-generated by steam_storage_lab.m\n');
        fprintf(fid, '// Do not edit by hand — re-run the lab instead.\n');
        fprintf(fid, '#pragma once\n\n');
        fprintf(fid, 'namespace storage {\n\n');
        fprintf(fid, 'constexpr size_t kChunkSize        = %d;  // bytes\n', ...
                tuned.chunkSizeBytes);
        fprintf(fid, 'constexpr double kDedupRatio      = %.4f;\n', ...
                tuned.dedupRatio);
        fprintf(fid, 'constexpr double kDeltaRatio      = %.4f;\n', ...
                tuned.deltaRatio);
        fprintf(fid, 'constexpr int    kCloudSyncSec    = %d;\n', ...
                tuned.cloudSyncSec);
        fprintf(fid, 'constexpr size_t kPrefetchBudget  = %dull * 1024;\n', ...
                tuned.prefetchBudgetKB);
        fprintf(fid, 'constexpr double kPrefetchFillPct = %.2f;\n', ...
                tuned.prefetchFillPct);
        fprintf(fid, '\n} // namespace storage\n');
        fclose(fid);
        fprintf('  Wrote %s\n', outFile);
        fprintf('  Include it from your C++ build to consume the results.\n');
    end

    print_section('DONE');
    fprintf('  Dedup ratio    : %.2fx\n', tuned.dedupRatio);
    fprintf('  Delta ratio    : %.2f%% of full build\n', ...
            100 * tuned.deltaRatio);
    fprintf('  Chunk size     : %s\n', human_bytes(tuned.chunkSizeBytes));
    fprintf('  Cloud sync     : every %d s\n', tuned.cloudSyncSec);
    fprintf('  Prefetch budget: %d KB (%.1f%% filled)\n', ...
            tuned.prefetchBudgetKB, tuned.prefetchFillPct);
    fprintf('\n');

end
% ======================================================================
%  Local functions
% ======================================================================

function print_section(t)
    fprintf('\n%s\n', repmat('=', 1, 64));
    fprintf('  %s\n', t);
    fprintf('%s\n', repmat('=', 1, 64));
end

function s = human_bytes(n)
    if n >= 1024*1024
        s = sprintf('%.2f MB', n / (1024*1024));
    elseif n >= 1024
        s = sprintf('%.2f KB', n / 1024);
    else
        s = sprintf('%d B', round(n));
    end
end

function [assets, pool] = make_library(nAssets, nBlocks, minSize, maxSize)
%MAKE_LIBRARY  Build a synthetic asset library with shared blocks.
%   Each asset is a concatenation of 2-6 blocks drawn from a shared pool.
%   Sharing is what makes dedup measurable.
    pool = cell(1, nBlocks);
    for i = 1:nBlocks
        sz = randi([minSize maxSize]);
        pool{i} = uint8(randi([0 255], 1, sz));
    end
    assets = struct('name', {}, 'data', {}, 'size', {});
    for a = 1:nAssets
        k = randi([2 6]);
        picks = randi(nBlocks, 1, k);
        data = uint8([]);
        for p = picks
            data = [data, pool{p}];  %#ok<AGROW>  small enough here
        end
        assets(end+1) = struct('name', sprintf('asset_%03d.bin', a), ...
                               'data', data, 'size', numel(data));
    end
end

function h = hash_bytes(b)
%HASH_BYTES  Stable hex key for a uint8 buffer.
%   Uses bundled Java SHA-1 when available, otherwise a 32-bit FNV-1a
%   implemented carefully so 64-bit overflow does not silently break.
    persistent haveJava
    if isempty(haveJava)
        haveJava = ~isempty(which('java.security.MessageDigest'));
    end

    if haveJava
        md = java.security.MessageDigest.getInstance('SHA-1');
        md.update(uint8(b));
        raw = typecast(int8(md.digest()), 'uint8');
        h = lower(reshape(dec2hex(raw, 2)', 1, []));
        return;
    end

    % Pure-MATLAB FNV-1a 32-bit. Multiplication done in double with the
    % 64-bit intermediate split into 16-bit halves to stay exact.
    h32 = uint32(2166136261);
    p   = 16777619;
    for k = 1:numel(b)
        h32 = bitxor(h32, uint32(b(k)));
        hi  = double(bitshift(h32, -16));
        lo  = double(bitand(h32, uint32(65535)));
        t   = lo * p + mod(hi * p, 65536) * 65536;
        h32 = uint32(mod(t, 4294967296));
    end
    h = lower(dec2hex(h32, 8));
end

function d = chunk_diff(A, B, S)
%CHUNK_DIFF  Compare two buffers chunked at size S.
%   Returns the number of bytes and chunks that differ. Assumes A and B
%   are the same length (typical for a version bump).
    n = min(numel(A), numel(B));
    bytesChanged = 0;
    chunksChanged = 0;
    for off = 1:S:n
        hi = min(off + S - 1, n);
        a  = A(off:hi);
        b  = B(off:hi);
        if ~isequal(hash_bytes(a), hash_bytes(b))
            bytesChanged  = bytesChanged  + numel(a);
            chunksChanged = chunksChanged + 1;
        end
    end
    d = struct('bytesChanged', bytesChanged, 'chunksChanged', chunksChanged);
end

function [pick, bestVal, bestSize] = knapsack01(sizes, values, cap)
%KNAPSACK01  0/1 knapsack via dynamic programming.
%   sizes and values are equal-length vectors; cap is the integer budget.
    n = numel(sizes);
    dp = zeros(n + 1, cap + 1);
    for i = 1:n
        wi = sizes(i);
        vi = values(i);
        for w = 0:cap
            best = dp(i, w + 1);
            if wi <= w
                alt = dp(i, w - wi + 1) + vi;
                if alt > best, best = alt; end
            end
            dp(i + 1, w + 1) = best;
        end
    end

    bestVal  = dp(n + 1, cap + 1);
    bestSize = 0;
    pick     = [];
    w = cap;
    for i = n:-1:1
        if dp(i + 1, w + 1) ~= dp(i, w + 1)
            pick(end+1) = i;  %#ok<AGROW>
            w = w - sizes(i);
            bestSize = bestSize + sizes(i);
        end
    end
    pick = sort(pick);
end