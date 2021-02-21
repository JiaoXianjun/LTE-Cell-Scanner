function s = get_signal_from_sdr(sdr_board, freq, sampling_rate, bandwidth, num_second, gain1, gain2)

num_second_drop = 0.01; % 10ms

freq_str = num2str(round(freq/1e5)/10);
sampling_rate_str = num2str(round(sampling_rate/1e4)/100);
bandwidth_str = num2str(round(bandwidth/1e4)/100);

if strcmpi(sdr_board, 'hackrf')
    format_str = 'int8';
    filename_raw = 'hackrf_tmp.bin';
    delete(filename_raw);
    filename = ['f' freq_str '_s' sampling_rate_str '_bw' bandwidth_str '_' num2str(num_second) 's_hackrf.bin'];

    if gain1==-1
        gain1 = 40;
    end
    if gain2==-1
        gain2 = 40;
    end
    [~, gain1, gain2] = hackrf_gain_regulation(0, gain1, gain2);

    cmd_str = ['hackrf_transfer -r ' filename_raw ' -f ' num2str(freq) ' -s ' num2str(sampling_rate) ' -b ' num2str(bandwidth) ' -n ' num2str((num_second+num_second_drop)*sampling_rate) ' -l ' num2str(gain1) ' -a 1 -g ' num2str(gain2) ];
elseif strcmpi(sdr_board, 'rtlsdr')
	format_str = 'uint8';
	filename_raw = 'rtlsdr_tmp.bin';
    delete(filename_raw);
    filename = ['f' freq_str '_s' sampling_rate_str '_bw' bandwidth_str '_' num2str(num_second) 's_rtlsdr.bin'];

    if gain1==-1
        gain1 = 0;
    end

    cmd_str = ['rtl_sdr -f ' num2str(freq) ' -s ' num2str(sampling_rate) ' -n ' num2str((num_second+num_second_drop)*sampling_rate) ' -g ' num2str(gain1) ' ' filename_raw];
elseif strcmp(sdr_board, 'bladerf')
    format_str = 'int16';
    filename_raw = 'bladerf_tmp.bin';
elseif strcmp(sdr_board, 'usrp')
    format_str = 'int16';
    filename_raw = 'usrp_tmp.bin';
end

disp(cmd_str);
system(cmd_str);

fid_raw = fopen(filename_raw, 'r');
if fid_raw == -1
    disp(['Open ' filename_raw ' failed!']);
    return;
end
a = fread(fid_raw, inf, format_str);
fclose(fid_raw);

a = a( ((num_second_drop*sampling_rate*2) + 1):end); % drop the unstable period

if strcmpi(sdr_board, 'hackrf')
    s = (a(1:2:end) + 1i.*a(2:2:end))./128;
elseif strcmpi(sdr_board, 'rtlsdr')
	s = raw2iq(a);
elseif strcmp(sdr_board, 'bladerf')
    s = (a(1:2:end) + 1i.*a(2:2:end))./(2^16);
elseif strcmp(sdr_board, 'usrp')
    s = (a(1:2:end) + 1i.*a(2:2:end))./(2^16);
end

disp(filename);
fid = fopen(filename, 'w');
if fid_raw == -1
    disp(['Create ' filename ' failed!']);
    return;
end
fwrite(fid, a, format_str);
fclose(fid);
