% Jiao Xianjun (putaoshu@gmail.com; putaoshu@msn.com)
% LTE_DL_receiver.m
% Use hackrf to process all 20MHz LTE bandwidth.
% See also README in root directory, ../test, ../../rtl-sdr-LTE/scan-capture/.

% Use commands in test/cap_LTE_with_HACKRF.txt to capture 20MHz LTE signal,
% then use this script to process it by setting correct "filename".

% Some scripts are borrowed from:
% https://github.com/JiaoXianjun/LTE-Cell-Scanner
% https://github.com/JiaoXianjun/rtl-sdr-LTE
% https://github.com/Evrytania/LTE-Cell-Scanner
% https://github.com/Evrytania/Matlab-Library
% https://github.com/JiaoXianjun/multi-rtl-sdr-calibration

% clear all;
function LTE_DL_receiver(varargin)
close all;
warning('off','all');

sampling_carrier_twist = 0; % ATTENTION! If this is 1, make sure fc is aligned with bin file!!!
num_radioframe = 8; % each radio frame length 10ms. MIB period is 4 radio frame
raw_sampling_rate = 19.2e6; % constrained by hackrf board

pss_peak_max_reserve = 2;
num_pss_period_try = 1;
combined_pss_peak_range = -1;
par_th = 8.5;
num_peak_th = 1/2;

if nargin == 0
    % ------------------------------------------------------------------------------------
    % % bin file captured by hackrf_transfer  
%     filename = '../regression_test_signal_file/f2585_s19.2_bw20_0.08s_hackrf_home.bin'; fc = 2585e6;
%     filename = '../regression_test_signal_file/f2585_s19.2_bw20_0.08s_hackrf_home_should.bin'; fc = 2585e6;
%     filename = '../regression_test_signal_file/f1852.5_s19.2_bw20_0.08s_hackrf_home.bin'; fc = 1852.5e6;
%     filename = '../regression_test_signal_file/f2565_s19.2_bw20_1s_hackrf_tsinghua.bin';  fc = 2565e6;
%     filename = '../regression_test_signal_file/f2585_s19.2_bw20_1s_hackrf_tsinghua.bin';  fc = 2585e6;
%     filename = '../regression_test_signal_file/f2360_s19.2_bw20_1s_hackrf.bin'; fc = 2360e6;
%     filename = '../regression_test_signal_file/f2360_s19.2_bw20_0.08s_hackrf.bin'; fc = 2360e6;
    filename = '../regression_test_signal_file/f2585_s19.2_bw20_1s_hackrf.bin'; fc = 2585e6;
%     filename = '../regression_test_signal_file/f2585_s19.2_bw20_1s_hackrf1.bin'; fc = 2585e6;
%     filename = '../regression_test_signal_file/f1860_s19.2_bw20_1s_hackrf_home1.bin'; fc = 1860e6;
%     filename = '../regression_test_signal_file/f1860_s19.2_bw20_1s_hackrf_home.bin'; fc = 1860e6;
%     filename = '../regression_test_signal_file/f1890_s19.2_bw20_1s_hackrf_home.bin'; fc = 1890e6;
%     filename = '../regression_test_signal_file/f1890_s19.2_bw20_1s_hackrf_home1.bin'; fc = 1890e6;
%     filename = '../regression_test_signal_file/f2605_s19.2_bw20_0.08s_hackrf_home.bin'; fc = 2605e6;
elseif nargin == 3 % freq lna_gain vga_gain
    freq_real = str2double(varargin{1})*1e6;
    lna_gain = str2double(varargin{2});
    vga_gain = str2double(varargin{3});
    [~, lna_gain_new, vga_gain_new] = hackrf_gain_regulation(0, lna_gain, vga_gain);
    
    filename_raw = 'hackrf_live_tmp.bin';
    delete(filename_raw);
    
    cmd_str = ['hackrf_transfer -r ' filename_raw ' -f ' num2str(freq_real) ' -s ' num2str(raw_sampling_rate) ' -b 20000000 -n ' num2str((num_radioframe*10+10)*(1e-3)*raw_sampling_rate) ' -l ' num2str(lna_gain_new) ' -a 1 -g ' num2str(vga_gain_new) ];
    disp(cmd_str);
    system(cmd_str);
    filename = ['f' num2str(varargin{1}) '_s19.2_bw20_0.08s_hackrf_runtime.bin'];
    fid_raw = fopen(filename_raw, 'r');
    if fid_raw == -1
        disp('Open hackrf_live_tmp.bin failed!');
        return;
    end
    a = fread(fid_raw, inf, 'int8');
    fclose(fid_raw);
    
    fid = fopen(filename, 'w');
    if fid_raw == -1
        disp(['Create ' filename ' failed!']);
        return;
    end
    fwrite(fid, a( (((10e-3)*raw_sampling_rate*2) + 1):end), 'int8');
    fclose(fid);
    clear a;
    
    fc = freq_real;
elseif nargin == 1
    filename = varargin{1};
    fc = get_frequency_carrier_from_filename(filename);
else
    disp('If there are parameters, the number of parameters must be 3: freq(MHz) lna_gain vga_gain');
    return;
end

disp(' ');
disp(filename);
disp(' ');   

sampling_rate = 30.72e6;
sampling_rate_pbch = sampling_rate/16; % LTE spec. 30.72MHz/16.

coef_pbch = fir1(254, (0.18e6*6+150e3)/raw_sampling_rate); %freqz(coef_pbch, 1, 1024);
coef_8x_up = fir1(254, 20e6/(raw_sampling_rate*8)); %freqz(coef_8x_up, 1, 1024);

% DS_COMB_ARM = 2;
% FS_LTE = 30720000;
% thresh1_n_nines=12;
% rx_cutoff=(6*12*15e3/2+4*15e3)/(FS_LTE/16/2);
% THRESH2_N_SIGMA = 3;

% f_search_set = 20e3:5e3:30e3; % change it wider if you don't know pre-information
f_search_set = -140e3:5e3:135e3;

if isempty(dir([filename(1:end-4) '.mat'])) || nargin == 3
    r_raw = get_signal_from_bin(filename, inf, 'hackrf');
    r_raw = r_raw - mean(r_raw); % remove DC

    figure(1);
%     show_signal_time_frequency(r_20M, sampling_rate, 180e3);
    show_signal_time_frequency(r_raw(1 : (25e-3*raw_sampling_rate)), raw_sampling_rate, 50e3); drawnow;
    figure(2);
    show_time_frequency_grid_raw(r_raw(1 : (25e-3*raw_sampling_rate)), raw_sampling_rate); drawnow;
    
    r_pbch = filter_wo_tail(r_raw(1 : (80e-3*raw_sampling_rate)), coef_pbch.*5, sampling_rate_pbch/raw_sampling_rate);
    r_20M = filter_wo_tail(r_raw(1 : (80e-3*raw_sampling_rate)), coef_8x_up.*8, 8);
    r_20M = r_20M(1:5:end);
    
    
    [cell_info, r_pbch, r_20M] = CellSearch(r_pbch, r_20M, f_search_set, fc, sampling_carrier_twist, pss_peak_max_reserve, num_pss_period_try, combined_pss_peak_range, par_th, num_peak_th);
    
    r_pbch = r_pbch.';
    r_20M = r_20M.';
    save([filename(1:end-4) '.mat'], 'r_pbch', 'r_20M', 'cell_info');
else
    load([filename(1:end-4) '.mat']);
%     [cell_info, r_pbch, r_20M] = CellSearch(r_pbch, r_20M, f_search_set, fc);
end

% cell_info
uldl_str = [ ...
        '|D|S|U|U|U|D|S|U|U|U|'; ...
        '|D|S|U|U|D|D|S|U|U|D|'; ...
        '|D|S|U|D|D|D|S|U|D|D|'; ...
        '|D|S|U|U|U|D|D|D|D|D|'; ...
        '|D|S|U|U|D|D|D|D|D|D|';
        '|D|S|U|D|D|D|D|D|D|D|';
        '|D|S|U|U|U|D|S|U|U|D|'
        ];
tic;
pcfich_corr = -1;
pcfich_info = -1;
for cell_idx = 1 : length(cell_info)
% for cell_idx = 1 : 1
    cell_tmp = cell_info(cell_idx);
    [tfg, tfg_timestamp, cell_tmp]=extract_tfg(cell_tmp,r_20M,fc,sampling_carrier_twist, cell_tmp.n_rb_dl);
%     [tfg_comp, tfg_comp_timestamp, cell_tmp]=tfoec(cell_tmp, tfg, tfg_timestamp, fc, sampling_carrier_twist, cell_tmp.n_rb_dl);
%     cell_tmp=decode_mib(cell_tmp,tfg_comp(:, 565:636));
    
    n_symb_per_subframe = 2*cell_tmp.n_symb_dl;
    n_symb_per_radioframe = 10*n_symb_per_subframe;
    num_radioframe = floor(size(tfg,1)/n_symb_per_radioframe);
    num_subframe = num_radioframe*10;
    pdcch_info = cell(1, num_subframe);
    pcfich_info = zeros(1, num_subframe);
    pcfich_corr = zeros(1, num_subframe);
    uldl_cfg = zeros(1, num_radioframe);
    
    nSC = cell_tmp.n_rb_dl*12;
    n_ports = cell_tmp.n_ports;
    
    tfg_comp_radioframe = zeros(n_symb_per_subframe*10, nSC);
    ce_tfg = NaN(n_symb_per_subframe, nSC, n_ports, 10);
    np_ce = zeros(10, n_ports);
    % % ----------------following process radio frame by radio frame-------------------
    for radioframe_idx = 1 : num_radioframe
        
        subframe_base_idx = (radioframe_idx-1)*10;
        
        % % channel estimation and decode pcfich
        for subframe_idx = 1 : 10
            sp = (subframe_base_idx + subframe_idx-1)*n_symb_per_subframe + 1;
            ep = sp + n_symb_per_subframe - 1;

            [tfg_comp, ~, ~] = tfoec_subframe(cell_tmp, subframe_idx-1, tfg(sp:ep, :), tfg_timestamp(sp:ep), fc, sampling_carrier_twist);
            tfg_comp_radioframe( (subframe_idx-1)*n_symb_per_subframe+1 : subframe_idx*n_symb_per_subframe, :) = tfg_comp;
            
            % Channel estimation
            for i=1:n_ports
                [ce_tfg(:,:,i, subframe_idx), np_ce(subframe_idx, i)] = chan_est_subframe(cell_tmp, subframe_idx-1, tfg_comp, i-1);
            end

            % pcfich decoding
            [pcfich_info(subframe_base_idx+subframe_idx), pcfich_corr(subframe_base_idx+subframe_idx)] = decode_pcfich(cell_tmp, subframe_idx-1, tfg_comp, ce_tfg(:,:,:, subframe_idx));
        end
        
        % identify uldl_cfg if TDD mode
        cell_tmp = get_uldl_cfg(cell_tmp, pcfich_info( (subframe_base_idx+1) : (subframe_base_idx+10) ));
        uldl_cfg(radioframe_idx) = cell_tmp.uldl_cfg;
        sfn = mod(cell_tmp.sfn+radioframe_idx-1, 1023);
        cell_info_post_str = [ ' CID-' num2str(cell_tmp.n_id_cell) ' nPort-' num2str(cell_tmp.n_ports) ' CP-' cell_tmp.cp_type ' PHICH-DUR-' cell_tmp.phich_dur '-RES-' num2str(cell_tmp.phich_res)];
        if cell_tmp.uldl_cfg >= 0 % TDD and valid pcfich/UL-DL-PATTERN detected
            disp(['TDD SFN-' num2str(sfn) ' ULDL-' num2str(cell_tmp.uldl_cfg) '-' uldl_str(cell_tmp.uldl_cfg+1,:) cell_info_post_str]);
            title_str = ['TDD SFN-' num2str(sfn) ' ULDL-' num2str(cell_tmp.uldl_cfg) cell_info_post_str];
        elseif cell_tmp.uldl_cfg == -2 % FDD and valid pcfich/UL-DL-PATTERN detected
            disp(['FDD SFN-' num2str(sfn) ' ULDL-0: D D D D D D D D D D' cell_info_post_str]);
            title_str = ['FDD SFN-' num2str(sfn) ' ULDL-0' cell_info_post_str];
        end
        
        figure(10);
        a = abs(tfg_comp_radioframe)';
        subplot(2,1,1); pcolor(a); shading flat; colorbar; title(title_str); xlabel('OFDM symbol idx'); ylabel('subcarrier idx'); drawnow;
        subplot(2,1,2); plot(a); drawnow;
        savefig([num2str(radioframe_idx) '.fig']);
        clear a;
        
        % % decode pdcch
        for subframe_idx = 1 : 10
            tfg_comp = tfg_comp_radioframe( (subframe_idx-1)*n_symb_per_subframe+1 : subframe_idx*n_symb_per_subframe, :);
            [sc_map, reg_info] = get_sc_map(cell_tmp, pcfich_info(subframe_base_idx+subframe_idx), subframe_idx-1);
            pdcch_info{subframe_base_idx+subframe_idx} = decode_pdcch(cell_tmp, reg_info, subframe_idx-1, tfg_comp, ce_tfg(:,:,:, subframe_idx), np_ce(subframe_idx,:));
            disp(['SF' num2str(subframe_idx-1) ' PHICH' num2str(reg_info.n_phich_symb) ' PDCCH' num2str(reg_info.n_pdcch_symb) ' RNTI: ' pdcch_info{subframe_base_idx+subframe_idx}.rnti_str]);
            if ~isempty(pdcch_info{subframe_base_idx+subframe_idx}.si_rnti_info)
                num_info = size(pdcch_info{subframe_base_idx+subframe_idx}.si_rnti_info,1);
                for info_idx = 1 : num_info
                    format1A_bits = pdcch_info{subframe_base_idx+subframe_idx}.si_rnti_info(info_idx,:);
                    format1A_location = pdcch_info{subframe_base_idx+subframe_idx}.si_rnti_location(info_idx,:);
                    [dci_str, dci_info] = parse_DCI_format1A(cell_tmp, 0, format1A_bits);
                    disp(['    PDCCH   No.' num2str(format1A_location(1)) '  ' num2str(format1A_location(2)) 'CCE: ' dci_str]);
%                     syms = decode_pdsch(cell_tmp, reg_info, dci_info, subframe_idx-1, tfg_comp, ce_tfg(:,:,:, subframe_idx), np_ce(subframe_idx,:), 0);
%                     figure(3); plot(real(syms), imag(syms), 'r.');
                    [sib_info, ~] = decode_pdsch(cell_tmp, reg_info, dci_info, subframe_idx-1, tfg_comp, ce_tfg(:,:,:, subframe_idx), np_ce(subframe_idx,:));
                    parse_SIB(sib_info);
%                     disp(['SIB crc' num2str(sib_info.blkcrc) ': ' num2str(sib_info.bits)]);
%                     figure(4); plot(real(syms), imag(syms), 'b.');
%                     if mod(sfn, 2) == 0 && subframe_idx==6
%                         title('raw SIB1 PDSCH');  xlabel('real'); ylabel('imag'); drawnow;
%                     else
%                         title('raw SIBx PDSCH');  xlabel('real'); ylabel('imag'); drawnow;
%                     end
                end
            end
%             figure(5); plot_sc_map(sc_map, tfg_comp);
        end
        
    end
    
    disp(num2str(pcfich_corr));
    sf_set = find(pcfich_info>0);
    val_set = pcfich_info(pcfich_info>0);
    disp(['subframe  ' num2str(sf_set)]);
    disp(['num pdcch ' num2str(val_set)]);

end

toc

% subplot(4,1,1); plot(pcfich_corr); axis tight;
% subplot(4,1,2); plot(sf_set, val_set, 'b.-'); axis tight;
% subplot(4,1,3);
% a = zeros(1, max(sf_set)); a(sf_set) = 1;
% pcolor([a;a]); shading faceted;  axis tight;
% subplot(4,1,4); plot(uldl_cfg);
