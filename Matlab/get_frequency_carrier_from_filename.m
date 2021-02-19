
function fc = get_frequency_carrier_from_filename(filename)
fc = -1;
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
