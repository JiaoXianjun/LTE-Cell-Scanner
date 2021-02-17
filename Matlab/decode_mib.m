function peak_out=decode_mib(peak,tfg)

% Try various combinations of antenna ports and frame timings to attempt
% to decode the MIB.

% Copyright 2012 Evrytania LLC (http://www.evrytania.com)
%
% Written by James Peroulas <james@evrytania.com>
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU Affero General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU Affero General Public License for more details.
%
% You should have received a copy of the GNU Affero General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>.

isOctave = exist('OCTAVE_VERSION', 'builtin') ~= 0;

% for matlab native viterbi decoder
if ~isOctave
  Hdec = comm.ViterbiDecoder( poly2trellis(7,[133 171 165]), 'TerminationMethod', 'Terminated');
end

peak_out=peak;

% Local shortcuts
n_id_1=peak.n_id_1;
n_id_2=peak.n_id_2;
cp_type=peak.cp_type;

% Derive some values
n_ofdm=size(tfg,1);
if (strcmpi(cp_type,'normal'))
  n_symb_dl=7;
elseif (strcmpi(cp_type,'extended'))
  n_symb_dl=6;
else
 error('Check code...');
end
n_id_cell=n_id_2+3*n_id_1;

peak_out.n_id_cell = n_id_cell;

% Channel estimation
ce_tfg=NaN(n_ofdm,72,4);
np_pre = NaN(1, 4);
[ce_tfg(:,:,1), np_pre(1)]=chan_est(peak,tfg,0,6);
[ce_tfg(:,:,2), np_pre(2)]=chan_est(peak,tfg,1,6);
[ce_tfg(:,:,3), np_pre(3)]=chan_est(peak,tfg,2,6);
[ce_tfg(:,:,4), np_pre(4)]=chan_est(peak,tfg,3,6);

mmse_flag = 0;
% Try various numbers of ports and various frame timing offsets
for frame_timing_guess=0:3
  % Extract the 4 frames that will be used to find the PBCH
  %ofdm_sym_set=frame_timing_guess*10*2*n_symb_dl+1:10*2*n_symb_dl*(frame_timing_guess+4);
  ofdm_sym_set = frame_timing_guess*10*2*n_symb_dl+1;
  ofdm_sym_set = ofdm_sym_set:ofdm_sym_set+3*10*2*n_symb_dl+2*n_symb_dl-1;
  tfg_try = tfg(ofdm_sym_set,:);
  ce_try = ce_tfg(ofdm_sym_set,:,:);

  for frame_phase = 0:3
      % Extract symbols and channel estimates for the PBCH
      [pbch_sym, pbch_ce]=pbch_extract(peak, tfg_try, ce_try, frame_phase);
      %load override_pbch_ce.mat

      found=0;
      %disp('Check code here!!!');
      for n_ports=[1 2 4]
        % -----------equalization----------
        [syms, np] = LTE_SFBC_demod(pbch_sym, pbch_ce, np_pre, n_ports, mmse_flag);

        % Extract the bits
        e_est=deqam(syms,np,'QAM','LTE');
        % Unscramble
        scr=lte_pn(n_id_cell,length(e_est));
        e_est(scr==1)=1-e_est(scr==1);
        % Undo ratematching
        d_est=lte_conv_deratematch(e_est,40);
        % If identical CE smoothing is used, C code matches perfectly until
        % here.
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
        crc_est=lte_calc_crc(c_est(1:24),16);
        %c_est(25:end)
        % Apply CRC mask
        if (n_ports==2)
          crc_est=1-crc_est;
        elseif (n_ports==4)
          crc_est(2:2:end)=1-crc_est(2:2:end);
        end
        %crc_est
        % Success?
        if (all(crc_est==c_est(25:end)))
          found=1;
          break;
        end

      end

      if (found==1)
        break;
      end
      
  end
  
  if (found==1)
    break;
  end
  
end

% Record the extracted data if the MIB was properly decoded
if (found)
  disp('Found MIB!!!');
  %frame_timing_guess
  peak_out.n_ports=n_ports;
%   disp(num2str(c_est(1:24)));

  % Record system BW
  bw_packed=c_est(1)*4+c_est(2)*2+c_est(3);
  switch (bw_packed)
    case 0
      n_rb_dl=6;
    case 1
      n_rb_dl=15;
    case 2
      n_rb_dl=25;
    case 3
      n_rb_dl=50;
    case 4
      n_rb_dl=75;
    case 5
      n_rb_dl=100;
    otherwise
      n_rb_dl=NaN;
  end
  peak_out.n_rb_dl=n_rb_dl;

  % PHICH duration
  peak_out.phich_dur_val = c_est(4);
  if (c_est(4))
    peak_out.phich_dur='extended';
  else
    peak_out.phich_dur='normal';
  end

  % PHICH res
  phich_res=c_est(5)*2+c_est(6);
  phich_res_map=[1/6 1/2 1 2];
  peak_out.phich_res=phich_res_map(phich_res+1);

  % Calculate the SFN of the first frame in the tfg variable.
  sfn_bits=c_est(7:14);
  sfn=0;
  for t=1:length(sfn_bits)
    sfn=2*sfn+sfn_bits(t);
  end
  sfn=sfn*4 - (frame_timing_guess+frame_phase);
  sfn=mod(sfn,1024);
  peak_out.sfn=sfn;

end

