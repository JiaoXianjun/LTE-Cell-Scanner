function parse_SIB(sib_info)

if sib_info.blkcrc ~= 0
    disp('SIB CRC error!');
    return;
end

bits = sib_info.bits;

% isOctave = exist('OCTAVE_VERSION', 'builtin') ~= 0;

fid = fopen('tmp_sib_info.per', 'w');

% if isOctave
  len = length(bits);
  if mod(len, 8) > 0
    disp('SIB len error!');
    return;
  end
  bits_uint8 = reshape(bits, [8, len/8])';
  bits_uint8 = bi2de(bits_uint8, 'left-msb');
  fwrite(fid, bits_uint8, 'uint8', 'b');
% else
%   fwrite(fid, bits, 'ubit1', 'b');
% end

fclose(fid);

disp('Calling asn1c decoder (../asn1_test/LTE-BCCH-DL-SCH-decode/progname) for BCCH-DL-SCH-Message.');
cmd_str = '../asn1_test/LTE-BCCH-DL-SCH-decode/progname tmp_sib_info.per -p BCCH-DL-SCH-Message';
disp(cmd_str);

[~, cmdout] = system(cmd_str);

disp(cmdout);

% % % % ---------------------original---------------------------
% hex_str = char(convert_SIB_bin_to_HEX(bits));
% % disp(['PDCCH No.0 4CCE:
% disp(['    PDSCH   SIB  (HEX): ' hex_str]);
% 
% % 
% % % 36.331 BCCH-DL-SCH-Message
% % 
% % sib2 = [];
% % TypeAndInfo.type1 = {sib2};
% % systemInformation_r8 = {sib-TypeAndInfo};
% % 
% % criticalExtensions.type1 = {systemInformation_r8};
% % criticalExtensionsFuture = [];
% % criticalExtensions.type2 = {criticalExtensionsFuture};
% % 
% % systemInformation = {criticalExtensions};
% % 
% % PLMN_Identity = {mcc, mcc};
% % 
% % plmn_IdentityList = 
% % cellAccessRelatedInfo = {plmn_IdentityList, trackingAreaCode, cellIdentity, cellBarred, intraFreqReselection, csg_Indication, csg_Identity};
% % systemInformationBlockType1 = {cellAccessRelatedInfo, cellSelectionInfo, p_Max, freqBandIndicator, schedulingInfoList, tdd_Config, si_WindowLength, systemInfoValueTag, nonCriticalExtension};
% % 
% % c1.type1 = {systemInformation};
% % c1.type2 = {systemInformationBlockType1};
% % 
% % message.type1 = { c1 };
% % messageClassExtension = [];
% % message.type2 = { messageClassExtension };
% % 
% % BCCH-DL-SCH-Message = { message };
