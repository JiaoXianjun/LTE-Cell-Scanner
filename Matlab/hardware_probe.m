function hardware = hardware_probe
% Priority: hackrf, bladerf, usrp, rtlsdr
hardware = [];

[~, cmdout] = system('hackrf_info');
if ~isempty(strfind(cmdout, 'Part'))
    hardware = 'hackrf';
    return;
end

[~, cmdout] = system('bladeRF-cli -p');
if ~isempty(strfind(cmdout, 'Address'))
    hardware = 'bladerf';
    return;
end

[~, cmdout] = system('uhd_find_devices');
if ~isempty(strfind(cmdout, 'type'))
    hardware = 'usrp';
    return;
end

[~, cmdout] = system('rtl_biast');
if ~isempty(strfind(cmdout, 'tuner'))
    hardware = 'rtlsdr';
    return;
end

