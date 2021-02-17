function pdcch_info = pdcch_bit_level_proc(peak, e_est)
% The common search space would carry the DCIs that are common for all UEs. 
% For example, system information (using the SI-RNTI), paging (P-RNTI), PRACH responses (RA-RNTI), 
% or UL TPC commands (TPC-PUCCH/PUSCH-RNTI). The UE monitors the common search space 
% using aggregation level 4 and 8. Maximum number of CCEs present in common search space is 16.
% 
% The UE-specific search space can carry DCIs for UE-specific allocations using 
% the UE's assigned C-RNTI, semi-persistent scheduling (SPS C-RNTI),or initial allocation (temporary C-RNTI). 
% The UE monitors the UE-specific search space at all aggregation levels (1, 2, 4, and 8).

% according to 7.1 8.0 of 36.213, DCI formats in common search space are:
% 0/1A/1C/3/3A
% input 0-with-1A/0-with-1A-RA-C-RNTI/1A/1A-RA-C-RNTI/3-with-1A/3-with-1A-RA-C-RNTI/3A-with-1A/3A-with-1A-RA-C-RNTI
% to get_num_DCI_bits(), you will find there are only two types of length:
% 31 and 41.

isOctave = exist('OCTAVE_VERSION', 'builtin') ~= 0;

% for matlab native viterbi decoder
if ~isOctave
  Hdec = comm.ViterbiDecoder( poly2trellis(7,[133 171 165]), 'TerminationMethod', 'Terminated');
end

% -------blind search in common search space-----------
num_CCE = 16;  % common search space
L_set = [4 8]; % common search space

bits_set = L_set.*9.*4.*2;
info_bits_set = [ get_num_DCI_bits(peak, '1A', 0)  get_num_DCI_bits(peak, '1A-RA-C-RNTI', 0)] + 16;

max_num = length(L_set)*sum(num_CCE./L_set);
pdcch_info.rnti_str = zeros(max_num, 8);
num_format1A_bits = get_num_DCI_bits(peak, '1A', 0);
pdcch_info.si_rnti_info = zeros(max_num, num_format1A_bits); % format 1A
pdcch_info.si_rnti_location = zeros(max_num, 2); % format 1A
pdcch_info_count = 1;
si_rnti_count = 1;
for l = 1 : length(L_set)
    L = L_set(l);
    M = num_CCE/L;
    for m = 0 : (M-1)
        sp = m*bits_set(l) + 1;
        ep = sp + bits_set(l) - 1;
        e_est_tmp = e_est(sp:ep);
        for i = 1 : length(info_bits_set)
            num_bits = info_bits_set(i);

            % Undo ratematching
            d_est=lte_conv_deratematch(e_est_tmp, num_bits);
            
            % Viterbi decode
            if isOctave
                      c_est=lte_conv_decode(d_est);
            else
              llr = -log((d_est(:).')./((1-d_est(:)).'));
              llr(llr>300) = 300;
              llr(llr<-300) = -300;
              c_est = step(Hdec, [llr, llr, llr].').';
              c_est = c_est(length(d_est) + 1 : 2*length(d_est) );
            end
            
            % Calculate the received CRC
            crc_est=lte_calc_crc(c_est(1:(num_bits-16)),16);

            % Apply CRC mask
            b = xor(crc_est, c_est((end-15):end) );

            a = bi2de(b, 'left-msb');
            if a == hex2dec('FFFF')
                rnti_str = 'SI-RNTI ';
                pdcch_info.si_rnti_info(si_rnti_count,:) = c_est(1:(num_bits-16));
                pdcch_info.si_rnti_location(si_rnti_count,:) = [m, L];
                si_rnti_count = si_rnti_count + 1;
            elseif a == hex2dec('FFFE')
                rnti_str = ' P-RNTI ';
            elseif a == hex2dec('FFFD')
                rnti_str = ' M-RNTI ';
            elseif a >= 1 && a <= hex2dec('3C')
%                 rnti_str = ' X-RNTI ';
                rnti_str = [' 0x' dec2hex(a, 4) ' '];
            else
                rnti_str = -1;
            end
            
            if rnti_str ~= -1
                pdcch_info.rnti_str(pdcch_info_count, :) = rnti_str;
                pdcch_info_count = pdcch_info_count + 1;
            end
        end
    end
end

pdcch_info.rnti_str = pdcch_info.rnti_str(1:(pdcch_info_count-1),:).';
pdcch_info.rnti_str = char(pdcch_info.rnti_str(:).');

pdcch_info.si_rnti_info = pdcch_info.si_rnti_info(1:(si_rnti_count-1),:);
pdcch_info.si_rnti_location = pdcch_info.si_rnti_location(1:(si_rnti_count-1),:);
