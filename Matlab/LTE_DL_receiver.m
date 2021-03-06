% Xianjun Jiao (putaoshu@msn.com)
function LTE_DL_receiver(varargin)
% From IQ sample to PDSCH output and RRC SIB messages.

% Usage 1: Run without any argument. Change the code manually when "if nargin == 0"

% Usage 2: Run with sdr board. Input arguments: freq(MHz)
% -- Above will use default gain (AGC or fixed default value). If it doesn't work well, at most two gain values can be input after freq:
% -- Example: LTE_DL_receiver 2528 40 30 (Carrier frequency 2528Mhz; gain1 40dB; gain2 30dB)
% -- Hackrf uses gain1 as lna gain and gain2 as vga gain. Other boards pick only the 1st gain value.

% Usage 3: Run with pre-captured IQ file. Input argument: filename (Should follow style: f2585_s19.2_bw20_0.08s_hackrf.bin)

close all;
warning('off','all');

sampling_carrier_twist = 1; % ATTENTION! If this is 1, make sure fc is aligned with bin file!!!
num_radioframe = 8; % Each radio frame length 10ms. MIB period is 4 radio frame
num_second = num_radioframe*10e-3;
raw_sampling_rate = 19.2e6; % Constrained by hackrf board and LTE signal format (100RB). Rtlsdr uses 1.92e6 due to hardware limitation
nRB = 100;
sampling_rate = 30.72e6;
sampling_rate_pbch = sampling_rate/16; % LTE spec. 30.72MHz/16.
bandwidth = 20e6;

pss_peak_max_reserve = 2;
num_pss_period_try = 1;
combined_pss_peak_range = -1;
par_th = 8.5;
num_peak_th = 1/2;

sdr_board = [];
filename = [];

if nargin == 0
    filename = '../regression_test_signal_file/f1815.3_s19.2_bw20_0.08s_hackrf-1.bin';
elseif isstr(varargin{1})
    if ~isempty(strfind(varargin{1}, '.bin')) % Get IQ filename
        filename = varargin{1};
    else
        disp('Filename is not valid!');
        return;
    end
else % Detect sdr board and capture IQ to file
    sdr_board = hardware_probe;
    if isempty(sdr_board)
        disp('No sdr board found!');
        return;
    end
    
    fc = varargin{1}*1e6;
    
    gain1 = -1;
    gain2 = -1;
    if nargin >= 2
        gain1 = varargin{2};
    end
    if nargin >= 3
        gain2 = varargin{3};
    end
    
    if strcmpi(sdr_board, 'rtlsdr')
        raw_sampling_rate = 1.92e6;
        nRB = 6; % PBCH only. for rtlsdr
        bandwidth = 1.2e6;
    end
    r_raw = get_signal_from_sdr(sdr_board, fc, raw_sampling_rate, bandwidth, num_second, gain1, gain2);
end

if ~isempty(filename) % If need to read from bin file
    [fc, sdr_board] = get_freq_hardware_from_filename(filename);
    if isempty(fc) || isempty(sdr_board)
        disp([filename ' does not include valid frequency or hardware info!']);
        return;
    end
    disp(filename);
    
    r_raw = get_signal_from_bin(filename, inf, sdr_board);
    if strcmpi(sdr_board, 'rtlsdr')
        raw_sampling_rate = 1.92e6; % rtlsdr limited sampling rate
        nRB = 6; % PBCH only. for rtlsdr
    end
end
disp(['fc ' num2str(fc) '; IQ from ' sdr_board ' ' filename]);   

if ~strcmpi(sdr_board, 'rtlsdr')
    coef_pbch = fir1(254, (0.18e6*6+150e3)/raw_sampling_rate); %freqz(coef_pbch, 1, 1024);
    coef_8x_up = fir1(254, 20e6/(raw_sampling_rate*8)); %freqz(coef_8x_up, 1, 1024);
end

% --------------------------- Cell Search ---------------------------
% DS_COMB_ARM = 2;
% FS_LTE = 30720000;
% thresh1_n_nines=12;
% rx_cutoff=(6*12*15e3/2+4*15e3)/(FS_LTE/16/2);
% THRESH2_N_SIGMA = 3;

% f_search_set = 20e3:5e3:30e3; % change it wider if you don't know pre-information
f_search_set = -140e3:5e3:135e3;

if (~isempty(filename)) && exist([filename(1:end-4) '.mat'], 'file')
    load([filename(1:end-4) '.mat']);
%     [cell_info, r_pbch, r_20M] = CellSearch(r_pbch, r_20M, f_search_set, fc);
    for i=1:length(cell_info)
        peak = cell_info(i);
        if peak.duplex_mode == 1
            cell_mode_str = 'TDD';
        else
            cell_mode_str = 'FDD';
        end
        disp(['Cell ' num2str(i) ' information:--------------------------------------------------------']);
        disp(['            Cell mode: ' num2str(cell_mode_str)]);
        disp(['              Cell ID: ' num2str(peak.n_id_cell)]);
        disp(['   Num. eNB Ant ports: ' num2str(peak.n_ports)]);
        disp(['    Carrier frequency: ' num2str(fc/1e6) 'MHz']);
        disp(['Residual freq. offset: ' num2str(peak.freq_superfine/1e3) 'kHz']);
        disp(['       RX power level: ' num2str(10*log10(peak.pow))]);
        disp(['              CP type: ' peak.cp_type]);
        disp(['              Num. RB: ' num2str(peak.n_rb_dl)]);
        disp(['       PHICH duration: ' peak.phich_dur]);
        disp(['  PHICH resource type: ' num2str(peak.phich_res)]);
    end
else
    r_raw = r_raw - mean(r_raw); % remove DC

    figure(1);
%     show_signal_time_frequency(r_20M, sampling_rate, 180e3);
    show_signal_time_frequency(r_raw(1 : (25e-3*raw_sampling_rate)), raw_sampling_rate, 50e3); drawnow;
    figure(2);
    show_time_frequency_grid_raw(r_raw(1 : (25e-3*raw_sampling_rate)), raw_sampling_rate, nRB); drawnow;
    
    if strcmpi(sdr_board, 'rtlsdr')
        r_pbch = r_raw;
        r_20M = [];
    else
        r_pbch = filter_wo_tail(r_raw(1 : (80e-3*raw_sampling_rate)), coef_pbch.*5, sampling_rate_pbch/raw_sampling_rate);
        r_20M = filter_wo_tail(r_raw(1 : (80e-3*raw_sampling_rate)), coef_8x_up.*8, 8);
        r_20M = r_20M(1:5:end);
    end
    
    [cell_info, r_pbch, r_20M] = CellSearch(r_pbch, r_20M, f_search_set, fc, sampling_carrier_twist, pss_peak_max_reserve, num_pss_period_try, combined_pss_peak_range, par_th, num_peak_th);
    
    r_pbch = r_pbch.';
    r_20M = r_20M.';
    if ~isempty(filename) && ~isempty(cell_info)
        save([filename(1:end-4) '.mat'], 'r_pbch', 'r_20M', 'cell_info');
    end
end

if strcmpi(sdr_board, 'rtlsdr')
    disp('The sampling rate (1.92M) of rtlsdr can not support 100RB demodulation! End of program.');
    return;
end

% ----------------Decode PDSCH in wider bandwidth-------------------
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
    if isempty(tfg)
        continue;
    end
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
        subplot(2,1,1); pcolor(a); shading flat; title(['RE grid: ' title_str]); xlabel('OFDM symbol idx'); ylabel('subcarrier idx'); drawnow; %colorbar; 
        subplot(2,1,2); plot(a); xlabel('subcarrier idx'); ylabel('abs'); legend('diff color diff OFDM symbol'); grid on; title('Spectrum of each OFDM symbol'); drawnow; %title('color -- OFDM symbol');  
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
