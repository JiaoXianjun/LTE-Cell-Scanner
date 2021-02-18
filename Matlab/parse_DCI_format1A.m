function [dci_str, dci_info] = parse_DCI_format1A(peak, space_ind, bits)
% PDCCH with DCI format 1A, 1B, 1C and 1D have a type 2 resource allocation

n_rb_dl = peak.n_rb_dl;
% n_rb_ul = n_rb_dl;
duplex_mode = peak.duplex_mode;
% uldl_cfg = peak.uldl_cfg;

if space_ind ~= 0
    disp('Now only common space is supported (space_ind = 0)');
    return;
end

FLAG_len = 1;
VRB_len = 1;
RBassign_len = ceil( log2( n_rb_dl*(n_rb_dl+1)/2 ) );

MCS_len = 5;
if duplex_mode == 1
    HARQ_len = 4;
else
    HARQ_len = 3;
end
NEW_len = 1;
RV_len = 2;
TPC_len = 2;
if duplex_mode == 1
    DAI_len = 2;
else
    DAI_len = 0;
end

sp = 1;

FLAG = bi2de( bits(sp : sp+FLAG_len-1), 'left-msb');
sp = sp + FLAG_len;

if FLAG == 0
    dci_str = ['FLAG-' num2str(FLAG) 'Attention! This is a format0 DCI! Not 1A!'];
    return;
end

VRB = bi2de( bits(sp : sp+VRB_len-1), 'left-msb'); % value 0 indicates Localized and value 1 indicates Distributed VRB assignment
sp = sp + VRB_len;

if VRB == 0
    VRB_str = 'Localized VRB';
else
    VRB_str = 'Distributed VRB';
end

RBassign = bi2de( bits(sp : sp+RBassign_len-1), 'left-msb');
sp = sp + RBassign_len;

L_CRBs = floor(RBassign/n_rb_dl) + 1;
if (L_CRBs-1) < floor(n_rb_dl/2)
    RB_start = RBassign - n_rb_dl*(L_CRBs - 1);
else
    L_CRBs = n_rb_ld + 1 - floor(RBassign/n_rb_ld);
    if (L_CRBs-1) < floor(n_rb_dl/2)
        disp('Abnormal!');
        return;
    else
        RB_start = n_rb_dl*(n_rb_dl - L_CRBs + 1) + (n_rb_dl-1) - RBassign;
    end
end

RBassign_str = ['from RB' num2str(RB_start) ' to RB' num2str(RB_start+L_CRBs-1)];

MCS = bi2de( bits(sp : sp+MCS_len-1), 'left-msb');
sp = sp + MCS_len;

HARQ = bi2de( bits(sp : sp+HARQ_len-1), 'left-msb');
sp = sp + HARQ_len;

NEW = bi2de( bits(sp : sp+NEW_len-1), 'left-msb');
sp = sp + NEW_len;

RV = bi2de( bits(sp : sp+RV_len-1), 'left-msb');
sp = sp + RV_len;

TPC = bi2de( bits(sp : sp+TPC_len-1), 'left-msb');
TPC_LSB = bits(sp+TPC_len-1);
sp = sp + TPC_len;

if DAI_len == 0
    DAI = [];
else
    DAI = bi2de( bits(sp : sp+DAI_len-1), 'left-msb');
end

dci_str = [VRB_str ' ' RBassign_str ' MCS-' num2str(MCS) ...
      ' HARQ-' num2str(HARQ)  ' NEWind-' num2str(NEW) ...
       ' RV-' num2str(RV)  ' TPC-' num2str(TPC) ...
        ' DAI-' num2str(DAI)];

dci_info.FLAG = FLAG;
dci_info.VRB = VRB;
dci_info.RBassign = RBassign;
dci_info.MCS = MCS;
dci_info.HARQ = HARQ;
dci_info.NEW = NEW;
dci_info.RV = RV;
dci_info.TPC = TPC;
dci_info.DAI = DAI;

dci_info.RB_start = RB_start;
dci_info.L_CRBs = L_CRBs; 

% we only care about 1A now according to 36.212 5.3.3.1.3
if TPC_LSB == 0
    dci_info.N_1A_PRB = 2;
else
    dci_info.N_1A_PRB = 3;
end

if dci_info.MCS >=0 && dci_info.MCS <= 9
    dci_info.bits_per_sym = 2;
elseif dci_info.MCS >=10 && dci_info.MCS <= 16
    dci_info.bits_per_sym = 4;
elseif dci_info.MCS >=17 && dci_info.MCS <= 28
    dci_info.bits_per_sym = 6;
end
