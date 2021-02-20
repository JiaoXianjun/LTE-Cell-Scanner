function s = get_signal_from_sdr(sdr_board, freq, sampling_rate, bandwidth, num_second, gain1, gain2)

[~, lna_gain_new, vga_gain_new] = hackrf_gain_regulation(0, lna_gain, vga_gain);

if nargin == 5
    bin_filename = 'hackrf_live_tmp.bin';
elseif nargin == 6
    bin_filename = varargin{1};
else
    disp('Number of input parameters must be 3 or 4.');
    return;
end

cmd_str = ['hackrf_transfer -r ' bin_filename ' -f ' num2str(freq) ' -s ' num2str(sampling_rate) ' -n ' num2str(num_second*sampling_rate) ' -l ' num2str(lna_gain_new) ' -g ' num2str(vga_gain_new) ];

system(cmd_str);

s = get_signal_from_bin(bin_filename, inf, 'hackrf');

s = s - mean(s);
subplot(2,1,1); plot(real(s));
subplot(2,1,2); plot(imag(s));

% figure;
% plot(angle(s));
