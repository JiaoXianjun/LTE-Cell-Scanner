function [ce_tfg, np]=chan_est_subframe(peak, subframe_idx, tfg, port)

% make this work in manner of subframe by subframe -- Jiao Xianjun
% (putaoshu@gmail.com)

% add 100RB support. Jiao Xianjun(putaohsu@gmail.com)
% Perform channel estimation on a specific antenna port

% Copyright 2012 Evrytania LLC (http://www.evrytania.com)
%
% Written by James Peroulas <james@evrytania.com>
%
% This program is free software: you can redistribute it and/or modify
% it under the terms of the GNU Affero General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU Affero General Public License for more details.
%
% You should have received a copy of the GNU Affero General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>.

nRB = peak.n_rb_dl;

nSC = nRB*12;
nRS = nRB*2;

cp_type=peak.cp_type;
n_id_cell=peak.n_id_cell;

n_symb_dl = peak.n_symb_dl;

n_ofdm=size(tfg,1);

if (port<=1)
  rs_set=sort([1:n_symb_dl:n_ofdm n_symb_dl-3+1:n_symb_dl:n_ofdm]);
else
  rs_set=2:n_symb_dl:n_ofdm;
end
n_rs_ofdm=length(rs_set);

% subframe_idx = mod(subframe_idx, 10);
start_slot_idx = subframe_idx*2;

% Extract the raw channel estimates
ce_raw = NaN(n_rs_ofdm,nRS);
slot_num = start_slot_idx;
for t=1:n_rs_ofdm
  %slot_num
  [rs, shift]=rs_dl(slot_num,mod(rs_set(t)-1,n_symb_dl),port,n_id_cell,nRB,cp_type);
  if (t==1)
    shift_1=shift;
  elseif (t==2)
    shift_2=shift;
  end
  ce_raw(t,:)=tfg(rs_set(t),shift+1:6:end).*conj(rs);
  %keyboard
  if ((mod(t,2)==0)||(port>=2))
    slot_num=mod(slot_num+1,20);
  end
end

% Primitive, fixed filtering of the raw channel estimates.
ce_filt=NaN(n_rs_ofdm,nRS);
current_row_leftmost=shift_1<shift_2;
for t=1:n_rs_ofdm
  for k=1:nRS
    %total=0;
    %n_total=0;
    % Current time offset
    if (k==1)
      ind=1:2;
    elseif (k==nRS)
      ind=(nRS-1):nRS;
    else
      ind=k-1:k+1;
    end
    total=sum(ce_raw(t,ind));
    n_total=length(ind);

    % Add in the shifted rows
    if (shift_1==shift_2)
      % Ports 2/3
      ind=k-1:k+1;
    else
      % Ports 0/1
      if (current_row_leftmost)
        ind=k-1:k;
      else
        ind=k:k+1;
      end
    end
    ind(ind<1)=[];
    ind(ind>nRS)=[];
    % Previous time offset
    if (t~=1)
      total=total+sum(ce_raw(t-1,ind));
      n_total=n_total+length(ind);
    end
    % Next time offset
    if (t~=n_rs_ofdm)
      total=total+sum(ce_raw(t+1,ind));
      n_total=n_total+length(ind);
    end
    ce_filt(t,k)=total/n_total;
    %n_total
    %pause
  end
  current_row_leftmost=~current_row_leftmost;
end
%ce_filt=ce_raw;

% Estimate received noise power
np=sigpower(ce_filt(:)-ce_raw(:));
%np=udb10(-14)

% Interpolate to fill in the missing samples
X=repmat(transpose(rs_set),1,nRS);
Y=[shift_1+1:6:nSC; shift_2+1:6:nSC];
Y=repmat(Y,ceil(n_rs_ofdm/2),1);
Y=Y(1:n_rs_ofdm,:);

xq = transpose(1:n_ofdm);
yq = 1:nSC;
isOctave = exist('OCTAVE_VERSION', 'builtin') ~= 0;
if isOctave
  [xq,yq] = meshgrid(xq,yq);
end
ce_tfg=transpose(griddata(X,Y,ce_filt,xq,yq));

% Fill in NaN samples at the edges
first_finite=0;
last_finite=-1;
for t=1:n_ofdm
  isn=find(isfinite(ce_tfg(t,:)));
  if (isempty(isn))
    continue;
  end
  if (first_finite==0)
    first_finite=t;
  end
  last_finite=t;
  ce_tfg(t,1:isn(1)-1)=ce_tfg(t,isn(1));
  ce_tfg(t,isn(end)+1:end)=ce_tfg(t,isn(end));
end
for t=1:n_ofdm
  if (isfinite(ce_tfg(t,1)))
    break;
  end
  ce_tfg(t,:)=ce_tfg(first_finite,:);
end
for t=n_ofdm:-1:1
  if (isfinite(ce_tfg(t,1)))
    break;
  end
  ce_tfg(t,:)=ce_tfg(last_finite,:);
end

%warning('Check code here!!!');
%ce_tfg(:,1:4)=randn(n_ofdm,4);
%ce_tfg(:,end-3:end)=randn(n_ofdm,4);

