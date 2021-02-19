function s = get_signal_from_bin(filename, num_sample_read, dev)

fid = fopen(filename);

if fid == -1
    disp('get_signal_from_bin: Can not open file!');
    return;
end

if strcmpi(dev, 'hackrf')
    [s, count] = fread(fid, num_sample_read*2, 'int8');
    s = (s(1:2:end) + 1i.*s(2:2:end))./128;
elseif strcmpi(dev, 'rtlsdr')
    [s, count] = fread(fid, num_sample_read*2, 'uint8');
    s = raw2iq(s);
elseif strcmp(dev, 'bladerf')
    [s, count] = fread(fid, num_sample_read*2, 'int16');
    s = (s(1:2:end) + 1i.*s(2:2:end))./(2^16);
elseif strcmp(dev, 'usrp')
    disp('to be done');
    return
end

fclose(fid);

if num_sample_read~=inf && count ~= (num_sample_read*2)
    s = -1;
    clear s;
    disp('get_signal_from_bin: No enough samples in the file!');
    return;
end

