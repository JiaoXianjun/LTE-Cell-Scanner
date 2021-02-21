function s = get_signal_from_sdr(sdr_board, freq, sampling_rate, bandwidth, num_second, gain1, gain2, varargin)

if strcmpi(sdr_board, 'hackrf')
    format_str = 'int8';
    filename_raw = 'hackrf_live_tmp.bin';
elseif strcmpi(sdr_board, 'rtlsdr')
     format_str = 'uint8';
elseif strcmp(sdr_board, 'bladerf') || strcmp(dev, 'usrp')
     format_str = 'int16';
end

fid_raw = fopen(filename_raw, 'r');
if fid_raw == -1
    disp('Open filename_raw failed!');
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