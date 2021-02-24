function s = get_signal_from_sdr(sdr_board, freq, sampling_rate, bandwidth, num_second, gain1, gain2)

num_second_drop = 0.01; % 10ms

freq_str = num2str(round(freq/1e5)/10);
sampling_rate_str = num2str(round(sampling_rate/1e4)/100);
bandwidth_str = num2str(round(bandwidth/1e4)/100);
cmd_freq_str = num2str(freq);
cmd_sampling_rate_str = num2str(sampling_rate);
cmd_n_sample_str = num2str((num_second+num_second_drop)*sampling_rate);

filename_raw = [sdr_board '_tmp.bin'];
delete(filename_raw);
filename = ['f' freq_str '_s' sampling_rate_str '_bw' bandwidth_str '_' num2str(num_second) 's_' sdr_board '.bin'];

if strcmpi(sdr_board, 'hackrf')
    format_str = 'int8';

    if gain1==-1
        gain1 = 40;
    end
    if gain2==-1
        gain2 = 40;
    end
    [~, gain1, gain2] = hackrf_gain_regulation(0, gain1, gain2);

    cmd_str = ['hackrf_transfer -f ' cmd_freq_str ' -s ' cmd_sampling_rate_str ' -b ' num2str(bandwidth) ' -n ' cmd_n_sample_str ' -l ' num2str(gain1) ' -a 1 -g ' num2str(gain2) ' -r ' filename_raw];
elseif strcmpi(sdr_board, 'rtlsdr')
	format_str = 'uint8';

    if gain1==-1
        gain1 = 0;
    end

    cmd_str = ['rtl_sdr -f ' cmd_freq_str ' -s ' cmd_sampling_rate_str ' -n ' cmd_n_sample_str ' -g ' num2str(gain1) ' ' filename_raw];
elseif strcmp(sdr_board, 'bladerf')
    format_str = 'int16';
    
    if gain1==-1
        gain1 = 60;
    end
    if gain2==-1
        gain2 = 25;
    end
    
    fid_bladerf_script = fopen('bladerf.script', 'w');
    if fid_bladerf_script == -1
        disp('Create bladerf.script failed!');
        return;
    end
    fprintf(fid_bladerf_script, 'set frequency rx %d\n', freq);
    fprintf(fid_bladerf_script, 'set samplerate rx %d\n', sampling_rate);
    fprintf(fid_bladerf_script, 'set bandwidth rx %d\n', bandwidth);
    fprintf(fid_bladerf_script, 'set gain rx %d\n', gain1);
%     fprintf(fid_bladerf_script, 'set lnagain %d\n', gain1);
%     fprintf(fid_bladerf_script, 'set rxvga1 %d\n', gain2);
%     fprintf(fid_bladerf_script, 'set rxvga2 %d\n', 30);
%     fprintf(fid_bladerf_script, 'cal lms\n');
%     fprintf(fid_bladerf_script, 'cal dc rx\n');
    fprintf(fid_bladerf_script, 'rx config file=%s format=bin n=%s\n', filename_raw, cmd_n_sample_str);
    fprintf(fid_bladerf_script, 'rx start\n');
    fprintf(fid_bladerf_script, 'rx wait\n');
    fclose(fid_bladerf_script);
    
    cmd_str = 'bladeRF-cli -s bladerf.script';
elseif strcmp(sdr_board, 'usrp')
    format_str = 'int16';
    
    if gain1==-1
        gain1 = 100;
    end
    
    cmd_str = ['uhd_rx_cfile -f ' cmd_freq_str ' -r ' cmd_sampling_rate_str ' -N ' cmd_n_sample_str ' -s -g ' num2str(gain1) ' ' filename_raw];
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
