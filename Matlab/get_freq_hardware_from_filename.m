function [fc, hardware] = get_freq_hardware_from_filename(filename)
fc = [];
hardware = [];

sp = strfind(filename, '/');
if ~isempty(sp)
    filename = filename((sp(end)+1):end);
end

sp = strfind(filename, 'f');
if isempty(sp)
    disp('Can not find f !');
    return;
end

ep = strfind(filename, '_');
if isempty(ep)
    disp('Can not find _ !');
    return;
end

sp = sp(1) + 1;
ep = ep(1) - 1;
fc = str2double(filename(sp:ep))*1e6;

if ~isempty(strfind(filename, 'rtlsdr')) % only can be used for pbch decoding (1.92Msps)
    hardware = 'rtlsdr';
elseif ~isempty(strfind(filename, 'hackrf'))
    hardware = 'hackrf';
elseif ~isempty(strfind(filename, 'bladerf'))
    hardware = 'bladerf';
elseif ~isempty(strfind(filename, 'usrp'))
    hardware = 'usrp';
else
    disp('The filename does not have hardware information (rtlsdr/hackrf/bladerf/usrp)!');
end

