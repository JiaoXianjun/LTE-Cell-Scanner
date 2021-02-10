// Copyright 2012 Evrytania LLC (http://www.evrytania.com)
//
// Written by James Peroulas <james@evrytania.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Improved by Jiao Xianjun (putaoshu@gmail.com):
// 1. TD-LTE support
// 2. OpenCL to speedup
// 3. fast pre-search frequencies (external mixer/LNB support)
// 4. multiple tries at one frequency
// 5. .bin file recording and replaying
// 6. hackrf support

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <itpp/itbase.h>
#include <itpp/stat/misc_stat.h>
#include <boost/math/special_functions/gamma.hpp>
#include <list>
#include <sstream>
#include <curses.h>
#include <sys/time.h>
#include <signal.h>

#include "common.h"

#ifdef HAVE_RTLSDR
#include "rtl-sdr.h"
#endif

#ifdef HAVE_HACKRF
#include "hackrf.h"
#endif

#ifdef HAVE_BLADERF
#include <libbladeRF.h>
extern volatile bool do_exit;
#ifdef _MSC_VER
BOOL WINAPI
sighandler(int signum)
{
	if (CTRL_C_EVENT == signum) {
		fprintf(stdout, "Caught signal %d\n", signum);
		do_exit = true;
		return TRUE;
	}
	return FALSE;
}
#else
void sigint_callback_handler(int signum)
{
	fprintf(stdout, "Caught signal %d\n", signum);
	do_exit = true;
}
#endif
#endif // HAVE_BLADERF

#include "macros.h"
#include "lte_lib.h"
#include "constants.h"
#include "capbuf.h"
#include "filter_coef.h"
#include "itpp_ext.h"
#include "searcher.h"
#include "dsp.h"

using namespace itpp;
using namespace std;

uint8 verbosity=1;

// Simple usage screen.
void print_usage() {
  cout << "LTE CellSearch (" << BUILD_TYPE << ") help. 1.0 to " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_LEVEL << ": OpenCL/TDD/HACKRF/bladeRF/ext-LNB added by Jiao Xianjun(putaoshu@gmail.com)" << endl << endl;
  cout << "CellSearch -s start_frequency [optional_parameters]" << endl;
  cout << "  Basic options" << endl;
  cout << "    -h --help" << endl;
  cout << "      print this help screen" << endl;
  cout << "    -v --verbose" << endl;
  cout << "      increase status messages from program" << endl;
  cout << "    -b --brief" << endl;
  cout << "      reduce status messages from program" << endl;
  cout << "    -i --device-index N" << endl;
  cout << "      specify which attached RTLSDR device to use" << endl;
  cout << "    -a --opencl-platform N" << endl;
  cout << "      specify which OpenCL platform to use (default: 0)" << endl;
  cout << "    -g --gain G" << endl;
  cout << "      specify gain to hardware (rtl default 0(auto); HACKRF default 40; bladeRF default LNA-MAX VGA-66)" << endl;
  cout << "    -j --opencl-device N" << endl;
  cout << "      specify which OpenCL device of selected platform to use (default: 0)" << endl;
  cout << "    -w --filter-workitem N" << endl;
  cout << "      specify how many OpenCL workitems are used for the 1st dim of 6RB filter (you'd better use values like 2^n)" << endl;
  cout << "    -u --xcorr-workitem N" << endl;
  cout << "      specify how many OpenCL workitems are used for the PSS xcorr (you'd better use values like 2^n)" << endl;
  cout << "  Frequency search options:" << endl;
  cout << "    -s --freq-start fs" << endl;
  cout << "      frequency where cell search should start" << endl;
  cout << "    -e --freq-end fe" << endl;
  cout << "      frequency where cell search should end" << endl;
  cout << "    -n --num-try nt" << endl;
  cout << "      number of tries at each frequency/file (default: 1)" << endl;
  cout << "    -m --num-reserve N" << endl;
  cout << "      number of reserved frequency-ppm peak pairs in pre-search phase (default: 1)" << endl;
  cout << "  Dongle LO correction options:" << endl;
  cout << "    -t --twisted" << endl;
  cout << "      enable original sampling-carrier-twisted mode (default is disable and using carrier&sampling isolated pre-search to support external mixer/LNB)" << endl;
  cout << "    -p --ppm ppm" << endl;
  cout << "      crystal remaining PPM error" << endl;
  cout << "    -c --correction c" << endl;
  cout << "      crystal correction factor" << endl;
  cout << "  Capture buffer save/ load options:" << endl;
  cout << "    -z --recbin" << endl;
  cout << "      save captured data in the bin file. (only supports single frequency scanning)" << endl;
  cout << "    -y --loadbin" << endl;
  cout << "      used data in captured bin file. (only supports single frequency scanning)" << endl;
  cout << "    -r --record" << endl;
  cout << "      save captured data in the files capbuf_XXXX.it" << endl;
  cout << "    -l --load" << endl;
  cout << "      used data in capbuf_XXXX.it files instead of live data" << endl;
  cout << "    -d --data-dir dir" << endl;
  cout << "      directory where capbuf_XXXX.it files are located" << endl << endl;
  cout << "'c' is the correction factor to apply and indicates that if the desired" << endl;
  cout << "center frequency is fc, the RTL-SDR dongle should be instructed to tune" << endl;
  cout << "to freqency fc*c so that its true frequency shall be fc. Default: 1.0" << endl << endl;
  cout << "'ppm' is the remaining frequency error of the crystal. Default: - (search -140kHz to 135kHz)" << endl;
  cout << "" << endl;
  cout << "If the crystal has not been used for a long time, use the default values for" << endl;
  cout << "'ppm' and 'c' until a cell is successfully located. The program will return" << endl;
  cout << "a 'c' value that can be used in the future, assuming that the crystal's" << endl;
  cout << "frequency accuracy does not change significantly." << endl;
  cout << "" << endl;
  cout << "Even if a correction factor has been calculated, there is usually some" << endl;
  cout << "remaining frequency error in the crystal. Thus, although after a c value is" << endl;
  cout << "calculated the ppm value can be reduced, it can not be reduced to 0." << endl;
  cout << "10.0 is a reasiable ppm value to use after a correction factor has been" << endl;
  cout << "calculated." << endl;
}

// Parse the command line arguments and return optional parameters as
// variables.
// Also performs some basic sanity checks on the parameters.
void parse_commandline(
  // Inputs
  const int & argc,
  char * const argv[],
  // Outputs
  double & freq_start,
  double & freq_end,
  uint16 & num_try,
  bool & sampling_carrier_twist,
  double & ppm,
  double & correction,
  bool & save_cap,
  bool & use_recorded_data,
  string & data_dir,
  int & device_index,
  char * record_bin_filename,
  char * load_bin_filename,
  uint16 & opencl_platform,
  uint16 & opencl_device,
  uint16 & filter_workitem,
  uint16 & xcorr_workitem,
  uint16 & num_reserve,
  uint16 & num_loop,
  int16  & gain
) {
  // Default values
  freq_start=-1;
  freq_end=-1;
  num_try=1; // default number
  sampling_carrier_twist=false;
  ppm=0;
  correction=1;
  save_cap=false;
  use_recorded_data=false;
  data_dir=".";
  device_index=-1;
  opencl_platform = 0;
  opencl_device = 0;
  filter_workitem = 32;
  xcorr_workitem = 2;
  num_reserve = 2;
  num_loop = 0;
  gain = -9999;

  while (1) {
    static struct option long_options[] = {
      {"help",         no_argument,       0, 'h'},
      {"verbose",      no_argument,       0, 'v'},
      {"brief",        no_argument,       0, 'b'},
      {"freq-start",   required_argument, 0, 's'},
      {"freq-end",     required_argument, 0, 'e'},
      {"num-try",      required_argument, 0, 'n'},
      {"twisted",      no_argument,       0, 't'},
      {"ppm",          required_argument, 0, 'p'},
      {"correction",   required_argument, 0, 'c'},
      {"recbin",       required_argument, 0, 'z'},
      {"loadbin",      required_argument, 0, 'y'},
      {"record",       no_argument,       0, 'r'},
      {"load",         no_argument,       0, 'l'},
      {"data-dir",     required_argument, 0, 'd'},
      {"device-index", required_argument, 0, 'i'},
      {"opencl-platform", required_argument, 0, 'a'},
      {"gain",         required_argument, 0, 'g'},
      {"opencl-device", required_argument, 0, 'j'},
      {"filter-workitem", required_argument, 0, 'w'},
      {"xcorr-workitem", required_argument, 0, 'u'},
      {"num-reserve", required_argument, 0, 'm'},
      {"num-loop", required_argument, 0, 'k'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c = getopt_long (argc, argv, "hvbs:e:n:tp:c:z:y:rld:i:a:g:j:w:u:m:k:",
                     long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
      char * endp;
      case 0:
        // Code should only get here if a long option was given a non-null
        // flag value.
        cout << "Check code!" << endl;
        ABORT(-1);
        break;
      case 'h':
        print_usage();
        ABORT(-1);
        break;
      case 'v':
        verbosity=2;
        break;
      case 'b':
        verbosity=0;
        break;
      case 's':
        freq_start=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse start frequency" << endl;
          ABORT(-1);
        }
        break;
      case 'e':
        freq_end=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse end frequency" << endl;
          ABORT(-1);
        }
        break;
      case 'n':
        num_try=strtol(optarg,&endp,10);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse number of tries" << endl;
          ABORT(-1);
        }
        break;
      case 't':
        sampling_carrier_twist=true;
        break;
      case 'p':
        ppm=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse ppm value" << endl;
          ABORT(-1);
        }
        break;
      case 'c':
        correction=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse correction factor" << endl;
          ABORT(-1);
        }
        break;
      case 'r':
        save_cap=true;
        break;
      case 'l':
        use_recorded_data=true;
        break;
      case 'z':
        {
          int len_str = strlen(optarg);
          if (len_str<5)
          {
            cerr << "Error: record bin filename too short" << endl;
            ABORT(-1);
          }
          if ( (len_str<1) || (strcmp(optarg+len_str-4, ".bin")) )
          {
            cerr << "Error: could not parse record bin filename (must be .bin file)" << endl;
            ABORT(-1);
          }
          else
          {
            strcpy(record_bin_filename, optarg);
          }
        }
        break;
      case 'y':
        {
          int len_str = strlen(optarg);
          if (len_str<5)
          {
            cerr << "Error: load bin filename too short" << endl;
            ABORT(-1);
          }
          if ( (len_str<1) || (strcmp(optarg+len_str-4, ".bin")) )
          {
            cerr << "Error: could not parse load bin filename (must be .bin file)" << endl;
            ABORT(-1);
          }
          else
          {
            strcpy(load_bin_filename, optarg);
            //freq_start=9999e6; // fake
          }
        }
        break;
      case 'd':
        data_dir=optarg;
        break;
      case 'i':
        device_index=strtol(optarg,&endp,10);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse device index" << endl;
          ABORT(-1);
        }
        if (device_index<0) {
          cerr << "Error: device index cannot be negative" << endl;
          ABORT(-1);
        }
        break;
      case 'a':
        opencl_platform=strtol(optarg,&endp,10);
        break;
      case 'g':
        gain=strtol(optarg,&endp,10);
        break;
      case 'j':
        opencl_device=strtol(optarg,&endp,10);
        break;
      case 'w':
        filter_workitem=strtol(optarg,&endp,10);
      case 'u':
        xcorr_workitem=strtol(optarg,&endp,10);
        break;
      case 'm':
        num_reserve=strtol(optarg,&endp,10);
        break;
      case 'k':
        num_loop = strtol(optarg,&endp,10);
        break;
      case '?':
        /* getopt_long already printed an error message. */
        ABORT(-1);
      default:
        ABORT(-1);
    }
  }

  // Error if extra arguments are found on the command line
  if (optind < argc) {
    cerr << "Error: unknown/extra arguments specified on command line" << endl;
    ABORT(-1);
  }

  // Second order command line checking. Ensure that command line options
  // are consistent.
  if (freq_start==-1) {
//    if (!sampling_carrier_twist) {
      freq_start=9999e6; // fake
      cout << "Warning: Frequency not specified. Make sure you are working on captured file.\n";
//    } else {
//      cerr << "Error: must specify a start frequency. (Try --help)" << endl;
//      ABORT(-1);
//    }
  }
  // Start and end frequencies should be on a 100kHz raster.
  if (freq_start<1e6) {
    cerr << "Error: start frequency must be greater than 1MHz" << endl;
    ABORT(-1);
  }
  // Number of tries should be not less than 1.
  if (num_try<1) {
    cerr << "Error: number of tries at each frequency/file should be not less than 1" << endl;
    ABORT(-1);
  }
//  if (freq_start/100e3!=itpp::round(freq_start/100e3)) {
//    freq_start=itpp::round(freq_start/100e3)*100e3;
//    cout << "Warning: start frequency has been rounded to the nearest multiple of 100kHz" << endl;
//  }
  if (freq_end==-1) {
    freq_end=freq_start;
  }
  if (freq_end<freq_start) {
    cerr << "Error: end frequency must be >= start frequency" << endl;
    ABORT(-1);
  }
//  if (freq_end/100e3!=itpp::round(freq_end/100e3)) {
//    freq_end=itpp::round(freq_end/100e3)*100e3;
//    cout << "Warning: end frequency has been rounded to the nearest multiple of 100kHz" << endl;
//  }
  // PPM values should be positive an most likely less than 200 ppm.
  if (ppm<0) {
    cerr << "Error: ppm value must be positive" << endl;
    ABORT(-1);
  }
  if (ppm>200) {
    cout << "Warning: ppm value appears to be set unreasonably high" << endl;
  }
  // Warn if correction factor is greater than 1000 ppm.
  if (abs(correction-1)>1000e-6) {
    cout << "Warning: crystal correction factor appears to be unreasonable" << endl;
  }
  // Should never both read and write captured data from a file
  if (save_cap&&use_recorded_data) {
    cerr << "Error: cannot read and write captured data at the same time!" << endl;
    ABORT(-1);
  }
  // Should never both read from .it and read from .bin file.
  if ( use_recorded_data && (strlen(load_bin_filename)>4) ) {
    cerr << "Error: cannot read from .it and .bin file at the same time!" << endl;
    ABORT(-1);
  }

  if (verbosity>=1) {
    cout << "LTE CellSearch (" << BUILD_TYPE << ") beginning. 1.0 to " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_LEVEL << ": OpenCL/TDD/HACKRF/bladeRF/ext-LNB added by Jiao Xianjun(putaoshu@gmail.com)" << endl;

// //    frequency information will be displayed in main() function.
//    if (freq_start==freq_end) {
//      cout << "  Search frequency: " << freq_start/1e6 << " MHz" << endl;
//    } else {
//      cout << "  Search frequency range: " << freq_start/1e6 << "-" << freq_end/1e6 << " MHz" << endl;
//    }

//    if (sampling_carrier_twist) {
      cout << "  PPM: " << ppm << endl;
      stringstream temp;
      temp << setprecision(20) << correction;
      cout << "  correction: " << temp.str() << endl;
//    }
    if (save_cap)
      cout << "  Captured data will be saved in capbufXXXX.it files" << endl;
    if (use_recorded_data)
      cout << "  Captured data will be read from capbufXXXX.it files" << endl;
  }
}

// In high SNR environments, a cell may be detected on different carrier
// frequencies and with different frequency offsets. Keep only the cell
// with the highest received power.
void dedup(
  const vector < list<Cell> > & detected_cells,
  list <Cell> & cells_final
) {
  cells_final.clear();
  for (uint16 t=0;t<detected_cells.size();t++) {
    list <Cell>::const_iterator it_n=detected_cells[t].begin();
    while (it_n!=detected_cells[t].end()) {
      bool match=false;
      list <Cell>::iterator it_f=cells_final.begin();
      while (it_f!=cells_final.end()) {
        // Do these detected cells match and are they close to each other
        // in frequency?
        if (
          ((*it_n).n_id_cell()==(*it_f).n_id_cell()) &&
          (abs(((*it_n).fc_requested+(*it_n).freq_superfine)-((*it_f).fc_requested+(*it_f).freq_superfine))<1e6)
        ) {
          match=true;
          // Keep either the new cell or the old cell, but not both.
          if ((*it_n).pss_pow>(*it_f).pss_pow) {
            (*it_f)=(*it_n);
          }
          break;
        }
        ++it_f;
      }
      if (!match) {
        // This cell does not match any previous cells. Add this to the
        // final list of cells.
        cells_final.push_back((*it_n));
      }
      ++it_n;
    }
  }
}

// Helper function to assist in formatting frequency offsets.
string freq_formatter(
  const double & freq
) {
  stringstream temp;
  if (abs(freq)<998.0) {
    temp << setw(5) << setprecision(3) << freq << "h";
  } else if (abs(freq)<998000.0) {
    temp << setw(5) << setprecision(3) << freq/1e3 << "k";
  } else if (abs(freq)<998000000.0) {
    temp << setw(5) << setprecision(3) << freq/1e6 << "m";
  } else if (abs(freq)<998000000000.0) {
    temp << setw(5) << setprecision(3) << freq/1e9 << "g";
  } else if (abs(freq)<998000000000000.0) {
    temp << setw(5) << setprecision(3) << freq/1e12 << "t";
  } else {
    temp << freq;
  }
  return temp.str();
}

#ifdef HAVE_RTLSDR
// Open the RTLSDR device
int config_rtlsdr(
  const bool & sampling_carrier_twist,
  const double & correction,
  const int32 & device_index_cmdline,
  const double & fc,
  rtlsdr_device *& dev,
  double & fs_programmed,
  const int16 & gain
) {
  int32 device_index=device_index_cmdline;

  int8 n_rtlsdr=rtlsdr_get_device_count();
  if (n_rtlsdr==0) {
    cerr << "config_rtlsdr Error: no RTL-SDR USB devices found..." << endl;
//    ABORT(-1);
    return(1);
  }

  // Choose which device to use
  if ((n_rtlsdr==1)&&(device_index==-1)) {
    device_index=0;
  }
  if ((device_index<0)||(device_index>=n_rtlsdr)) {
    cerr << "config_rtlsdr Error: must specify which USB device to use with --device-index" << endl;
    cerr << "config_rtlsdr Found the following USB devices:" << endl;
    char vendor[256],product[256],serial[256];
    for (uint8 t=0;t<n_rtlsdr;t++) {
      rtlsdr_get_device_usb_strings(t,vendor,product,serial);
      cerr << "Device index " << t << ": [Vendor: " << vendor << "] [Product: " << product << "] [Serial#: " << serial << "]" << endl;
    }
//    ABORT(-1);
    return(1);
  }

  // Open device
  if (rtlsdr_open(&dev,device_index)<0) {
    cerr << "config_rtlsdr Error: unable to open RTLSDR device" << endl;
//    ABORT(-1);
    return(1);
  }

  double sampling_rate = 0;
//  if (sampling_carrier_twist)
    sampling_rate = (FS_LTE/16)*correction;
//  else
//    sampling_rate = 1920000;
  // Sampling frequency
  if (rtlsdr_set_sample_rate(dev,itpp::round(sampling_rate))<0) {
    cerr << "config_rtlsdr Error: unable to set sampling rate" << endl;
//    ABORT(-1);
    return(1);
  }

  // Calculate the actual fs that was programmed
  fs_programmed=(double)rtlsdr_get_sample_rate(dev);

  // Center frequency
  uint8 n_fail=0;
  while (rtlsdr_set_center_freq(dev,itpp::round(fc*correction))<0) {
    n_fail++;
    if (n_fail>=5) {
      cerr << "config_rtlsdr Error: unable to set center frequency" << endl;
//      ABORT(-1);
      return(1);
    }
    cerr << "config_rtlsdr Unable to set center frequency... retrying..." << endl;
    sleep(1);
  }

  int gain_tmp = gain;
  if (gain == -9999)
    gain_tmp = 0;

  if (gain_tmp==0) {
    // Turn on AGC
    if (rtlsdr_set_tuner_gain_mode(dev,0)<0) {
      cerr << "config_rtlsdr Error: unable to enter AGC mode" << endl;
  //    ABORT(-1);
      return(1);
    }
  } else {
    if (rtlsdr_set_tuner_gain_mode(dev,1)<0) {
      cerr << "config_rtlsdr Error: unable to enter manual gain mode" << endl;
      return(1);
    }

    if (rtlsdr_set_tuner_gain(dev, gain_tmp*10)<0) {
      cerr << "config_rtlsdr Error: unable to rtlsdr_set_tuner_gain" << endl;
      return(1);
    }
  }

  // Reset the buffer
  if (rtlsdr_reset_buffer(dev)<0) {
    cerr << "config_rtlsdr Error: unable to reset RTLSDR buffer" << endl;
//    ABORT(-1);
    return(1);
  }

  // Discard about 1.5s worth of data to give the AGC time to converge
  if (verbosity>=2) {
    cout << "config_rtlsdr Waiting for AGC to converge..." << endl;
  }
  uint32 n_read=0;
  int n_read_current;
#define BLOCK_SIZE 16*16384
  uint8 * buffer=(uint8 *)malloc(BLOCK_SIZE*sizeof(uint8));
  while (true) {
    if (rtlsdr_read_sync(dev,buffer,BLOCK_SIZE,&n_read_current)<0) {
      cerr << "config_rtlsdr Error: synchronous read failed" << endl;
//      ABORT(-1);
      return(1);
    }
    if (n_read_current<BLOCK_SIZE) {
      cerr << "config_rtlsdr Error: short read; samples lost" << endl;
//      ABORT(-1);
      return(1);
    }
    n_read+=n_read_current;
    if (n_read>2880000*2)
      break;
  }
  free(buffer);

  return(0);
}
#endif // HAVE_RTLSDR

#ifdef HAVE_HACKRF
// Open the HACKRF device
int config_hackrf(
  const bool & sampling_carrier_twist,
  const double & correction,
  const int32 & device_index_cmdline,
  const double & fc,
  hackrf_device * & dev,
  double & fs_programmed,
  const int16 & gain
) {

  unsigned int lna_gain=40; // default value
  unsigned int vga_gain=40; // default value
  if (gain!=-9999)
    vga_gain = (gain/2)*2;

  int result = hackrf_init();
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_init() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//		ABORT(-1);
    return(result);
	}

	result = hackrf_open(&dev);
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_open() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//		ABORT(-1);
    return(result);
	}

  double sampling_rate =  (FS_LTE/16)*correction;

  // Sampling frequency
  result = hackrf_set_sample_rate_manual(dev, sampling_rate, 1);
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_sample_rate_set() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//		ABORT(-1);
    return(result);
	}

  // Need to handle in the future
  fs_programmed=sampling_rate;

  result = hackrf_set_baseband_filter_bandwidth(dev, 1.45e6);
	if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_baseband_filter_bandwidth_set() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//		ABORT(-1);
		return(result);
	}

  result = hackrf_set_vga_gain(dev, vga_gain);
	result |= hackrf_set_lna_gain(dev, lna_gain);

  if( result != HACKRF_SUCCESS ) {
		printf("config_hackrf hackrf_set_vga_gain hackrf_set_lna_gain failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//		ABORT(-1);
		return(result);
	}

  // Center frequency
  result = hackrf_set_freq(dev, fc);
  if( result != HACKRF_SUCCESS ) {
    printf("config_hackrf hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//    ABORT(-1);
    return(result);
  }

  // test read samples from dev
  cvec capbuf(CAPLENGTH);
  double fc_programmed;
  rtlsdr_device *fake_rtlsdr_dev = NULL;
  bladerf_device *fake_bladerf_dev = NULL;
  capture_data(fc, 1, false, " ", false, " ", " ",fake_rtlsdr_dev,dev,fake_bladerf_dev,dev_type_t::HACKRF, capbuf, fc_programmed, fs_programmed,0);

  return(result);
}
#endif

#ifdef HAVE_BLADERF
static inline const char *backend2str(bladerf_backend b)
{
    switch (b) {
        case BLADERF_BACKEND_LIBUSB:
            return "libusb";
        case BLADERF_BACKEND_LINUX:
            return "Linux kernel driver";
        default:
            return "Unknown";
    }
}
// Open the BLADERF device
int config_bladerf(
  const bool & sampling_carrier_twist,
  const double & correction,
  const int32 & device_index_cmdline,
  const double & fc,
  bladerf_device * & dev,
  double & fs_programmed,
  const int16 & gain
) {
  bladerf_devinfo *devices = NULL;
  int n_devices = bladerf_get_device_list(&devices);

  if (n_devices < 0) {
    if (n_devices == BLADERF_ERR_NODEV) {
        printf("config_bladerf bladerf_get_device_list: No bladeRF devices found.\n");
    } else {
        printf("config_bladerf bladerf_get_device_list: Failed to probe for bladeRF devices: %s\n", bladerf_strerror(n_devices));
    }
		return(-1);
  }

  printf("init_board: %d bladeRF devices found! The 1st one will be used:\n", n_devices);
  printf("    Backend:        %s\n", backend2str(devices[0].backend));
  printf("    Serial:         %s\n", devices[0].serial);
  printf("    USB Bus:        %d\n", devices[0].usb_bus);
  printf("    USB Address:    %d\n", devices[0].usb_addr);

  int fpga_loaded;
  int status = bladerf_open(&dev, NULL);
  if (status != 0) {
    printf("config_bladerf bladerf_open: Failed to open bladeRF device: %s\n",
            bladerf_strerror(status));
    return(-1);
  }

  fpga_loaded = bladerf_is_fpga_configured(dev);
  if (fpga_loaded < 0) {
      printf("config_bladerf bladerf_is_fpga_configured: Failed to check FPGA state: %s\n",
                bladerf_strerror(fpga_loaded));
      status = -1;
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  } else if (fpga_loaded == 0) {
      printf("config_bladerf bladerf_is_fpga_configured: The device's FPGA is not loaded.\n");
      status = -1;
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  double sampling_rate =  (FS_LTE/16)*correction;
  unsigned int actual_sample_rate;
  status = bladerf_set_sample_rate(dev, BLADERF_MODULE_RX, (unsigned int)sampling_rate, &actual_sample_rate);
  if (status != 0) {
      printf("config_bladerf bladerf_set_sample_rate: Failed to set samplerate: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }
  fs_programmed = actual_sample_rate;

  unsigned int actual_bw;
  status = bladerf_set_bandwidth(dev, BLADERF_MODULE_RX, 1500000, &actual_bw);
  if (status != 0) {
      printf("config_bladerf bladerf_set_bandwidth: Failed to set bandwidth: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  int gian_set = (gain==-9999?66:gain);
  status = bladerf_set_gain(dev, BLADERF_MODULE_RX, gian_set);
  if (status != 0) {
      printf("config_bladerf bladerf_set_gain: Failed to set gain: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  bladerf_lna_gain actual_lna_gain;
  status = bladerf_get_lna_gain(dev, &actual_lna_gain);
  if (status != 0) {
      printf("config_bladerf bladerf_get_lna_gain: Failed to get lna gain: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }
  char *lna_gain_str = NULL;
  if (actual_lna_gain == BLADERF_LNA_GAIN_BYPASS) {
    lna_gain_str = (char *)"BLADERF_LNA_GAIN_BYPASS";
  } else if (actual_lna_gain == BLADERF_LNA_GAIN_MAX) {
    lna_gain_str = (char *)"BLADERF_LNA_GAIN_MAX";
  } else if (actual_lna_gain == BLADERF_LNA_GAIN_MID) {
    lna_gain_str = (char *)"BLADERF_LNA_GAIN_MID";
  } else if (actual_lna_gain == BLADERF_LNA_GAIN_UNKNOWN) {
    lna_gain_str = (char *)"BLADERF_LNA_GAIN_UNKNOWN";
  } else {
    lna_gain_str = (char *)"INVALID_BLADERF_LNA_GAIN";
  }

  int actual_vga1_gain;
  status = bladerf_get_rxvga1(dev, &actual_vga1_gain);
  if (status != 0) {
      printf("config_bladerf bladerf_get_rxvga1: Failed to get rxvga1 gain: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  int actual_vga2_gain;
  status = bladerf_get_rxvga2(dev, &actual_vga2_gain);
  if (status != 0) {
      printf("config_bladerf bladerf_get_rxvga2: Failed to get rxvga2 gain: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  int actual_total_vga_gain = actual_vga1_gain + actual_vga2_gain;

  status = bladerf_set_frequency(dev, BLADERF_MODULE_RX, (unsigned int)fc);
  if (status != 0) {
      printf("config_bladerf bladerf_set_frequency: Failed to set frequency: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  long unsigned int actual_frequency;
  status = bladerf_get_frequency(dev, BLADERF_MODULE_RX, &actual_frequency);
  if (status != 0) {
      printf("config_bladerf bladerf_get_frequency: Failed to read back frequency: %s\n",
              bladerf_strerror(status));
      if (dev!=NULL) {bladerf_close(dev); dev = NULL; return(-1);}
  }

  #ifdef _MSC_VER
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
  #else
    signal(SIGINT, &sigint_callback_handler);
    signal(SIGILL, &sigint_callback_handler);
    signal(SIGFPE, &sigint_callback_handler);
    signal(SIGSEGV, &sigint_callback_handler);
    signal(SIGTERM, &sigint_callback_handler);
    signal(SIGABRT, &sigint_callback_handler);
  #endif

// test read samples from dev
  cvec capbuf(CAPLENGTH);
  double fc_programmed;
  rtlsdr_device *fake_rtlsdr_dev = NULL;
  hackrf_device *fake_hackrf_dev = NULL;
  capture_data(fc, 1, false, " ", false, " ", " ",fake_rtlsdr_dev,fake_hackrf_dev,dev,dev_type_t::BLADERF, capbuf, fc_programmed, fs_programmed,0);

  printf("config_bladerf: set bladeRF to %fMHz %fMsps BW %fMHz %s VGA_GAIN %ddB (%d+%d).\n", (float)actual_frequency/1000000.0f, (float)actual_sample_rate/1000000.0f, (float)actual_bw/1000000.0f, lna_gain_str, actual_total_vga_gain, actual_vga1_gain, actual_vga2_gain);
  return(status);
}
#endif

// Main cell search routine.
int main(
  const int argc,
  char * const argv[]
) {
  // Command line parameters are stored here.
  double freq_start;
  double freq_end;
  uint16 num_try;
  bool sampling_carrier_twist;
  double ppm;
  double correction;
  bool save_cap;
  bool use_recorded_data;
  string data_dir;
  int device_index;
  char record_bin_filename[256] = {0};
  char load_bin_filename[256] = {0};
  int16  gain;
  uint16 opencl_platform;
  uint16 opencl_device;
  uint16 filter_workitem;
  uint16 xcorr_workitem;
  uint16 num_reserve;
  uint16 num_loop; // it is not so useful

  // Get search parameters from user
  parse_commandline(argc,argv,freq_start,freq_end,num_try,sampling_carrier_twist,ppm,correction,save_cap,use_recorded_data,data_dir,device_index, record_bin_filename, load_bin_filename,opencl_platform,opencl_device,filter_workitem,xcorr_workitem,num_reserve,num_loop,gain);

  // Open the USB device (if necessary).
  dev_type_t::dev_type_t dev_use = dev_type_t::UNKNOWN;
  rtlsdr_device * rtlsdr_dev=NULL;
  hackrf_device * hackrf_dev = NULL; // if HAVE_HACKRF isn't defined, hackrf_device will be asigned a fake type.
  bladerf_device * bladerf_dev = NULL; // if HAVE_BLADERF isn't defined, bladerf_device will be asigned a fake type.

  double fs_programmed = FS_LTE/16; // in case not initialized by config_rtlsdr
  double fc_programmed; // for correction frequency calculation
  double fc_requested, fc_requested_tmp, fc_programmed_tmp, fs_requested_tmp, fs_programmed_tmp;

  bool dongle_used = (!use_recorded_data) && (strlen(load_bin_filename)==0);
  if ( dongle_used && freq_start!=9999e6) {

    #ifdef HAVE_RTLSDR
    if ( config_rtlsdr(sampling_carrier_twist,correction,device_index,freq_start,rtlsdr_dev,fs_programmed,gain) == 0 ) {
      dev_use = dev_type_t::RTLSDR;
      cout << "RTLSDR device FOUND!\n";
    } else {
      cout << "RTLSDR device not FOUND!\n";
      ABORT(-1);
    }
    #endif

    #ifdef HAVE_HACKRF
    if ( config_hackrf(sampling_carrier_twist,correction,device_index,freq_start,hackrf_dev,fs_programmed,gain) == 0 ) {
      dev_use = dev_type_t::HACKRF;
      cout << "HACKRF device FOUND!\n";
    } else {
      cout << "HACKRF device not FOUND!\n";
      ABORT(-1);
    }
    #endif

    #ifdef HAVE_BLADERF
    if ( config_bladerf(sampling_carrier_twist,correction,device_index,freq_start,bladerf_dev,fs_programmed,gain) == 0 ) {
      dev_use = dev_type_t::BLADERF;
      cout << "BLADERF device FOUND!\n";
    } else {
      cout << "BLADERF device not FOUND!\n";
      ABORT(-1);
    }
    #endif

    if (dev_use == dev_type_t::RTLSDR) {
      #ifdef HAVE_RTLSDR
      fc_programmed_tmp = calculate_fc_programmed_in_context(freq_start, use_recorded_data, load_bin_filename, rtlsdr_dev);
      #endif // HAVE_RTLSDR
    } else if (dev_use == dev_type_t::HACKRF) {
      fc_programmed_tmp = freq_start;
    } else if (dev_use == dev_type_t::BLADERF) {
      fc_programmed_tmp = freq_start;
    } else {
      cout << "No valid device present or configured.\n";
      ABORT(-1);
    }

    cout << "Use  HW  begin with " << ( freq_start/1e6 ) << "MHz actual " << (fc_programmed_tmp/1e6) << "MHz " << fs_programmed << "MHz\n";
  } else {
    if (strlen(load_bin_filename)!=0) { // use captured bin file
      if ( read_header_from_bin( load_bin_filename, fc_requested_tmp, fc_programmed_tmp, fs_requested_tmp, fs_programmed_tmp) ) {
        cerr << "main: read_header_from_bin failed.\n";
        ABORT(-1);
      }
      if (fc_requested_tmp==NAN) { //  no valid header information is found
        cerr << "Neither frequency nor valid bin file header information is specified!\n";
        ABORT(-1);
      }
    } else if (use_recorded_data){ // use captured .it file

      stringstream filename;
      filename << data_dir << "/capbuf_" << setw(4) << setfill('0') << 0 << ".it";
      it_ifile itf(filename.str());

      itf.seek("fc");
      ivec fc_v;
      itf>>fc_v;

      itf.seek("fcp");
      ivec fc_p;
      itf>>fc_p;

      itf.seek("fsp");
      ivec fs_p;
      itf>>fs_p;

      itf.close();

      fc_requested_tmp = fc_v(0);
      fc_programmed_tmp = fc_p(0);
      fs_programmed_tmp = fs_p(0);

    }
    fs_programmed = fs_programmed_tmp;
    cout << "Use file begin with " << ( fc_requested_tmp/1e6 ) << "MHz actual " << (fc_programmed_tmp/1e6) << "MHz " << fs_programmed_tmp << "MHz\n";
  }

  if (use_recorded_data)
    num_try=1; // compatible to .it file case

  // Generate a list of center frequencies that should be searched and also
  // a list of frequency offsets that should be searched for each center
  // frequency.
  vec fc_search_set;
  double freq_correction = 0;
  if (freq_start!=9999e6) { // if frequency scanning range is specified
    fc_search_set=itpp_ext::matlab_range(freq_start,100e3,freq_end);
  } else { // if frequency scanning range is not specified. a file is as input
    if (strlen(load_bin_filename)!=0 || use_recorded_data) { // use captured file

        freq_start = fc_requested_tmp;
        freq_end = freq_start;
        fc_search_set.set_length(1, false);
        fc_search_set[0] = fc_requested_tmp;

    } else {
      cerr << "Neither frequency nor captured file is specified!\n";
      ABORT(-1);
    }
  }
  freq_correction = fc_programmed_tmp*(correction-1)/correction;

  cout << "    Search frequency: " << fc_search_set(0)/1e6 << " to " <<  fc_search_set( length(fc_search_set)-1 )/1e6 << " MHz" << endl;
  cout << "with freq correction: " << freq_correction/1e3 << " kHz" << endl;

  cmat pss_fo_set;// pre-generate frequencies offseted pss time domain sequence
  vec f_search_set;
  if (sampling_carrier_twist) { // original mode
    uint16 n_extra=0;
    if (ppm!=0)
      n_extra=floor_i((fc_search_set(0)*ppm/1e6+2.5e3)/5e3);
    else
      n_extra=28;

    f_search_set=to_vec(itpp_ext::matlab_range( -n_extra*5000,5000, (n_extra-1)*5000));
    // for graphic card which has limited mem, you should turn num_loop on if OpenCL reports -4: CL_MEM_OBJECT_ALLOCATION_FAILURE
//    if (num_loop == 0) // it is not so useful
//      num_loop = length(f_search_set)/2;
  } else {
    if (length(fc_search_set)==1) {//when only one frequency is specified, whole PPM range should be covered
      uint16 n_extra=0;
      if (ppm!=0)
        n_extra=floor_i((fc_search_set(0)*ppm/1e6+2.5e3)/5e3);
      else
        n_extra=28; //-140kHz to 135kHz

      f_search_set=to_vec(itpp_ext::matlab_range( -n_extra*5000,5000, (n_extra-1)*5000));
    } else {
      // since we have frequency step is 100e3, why not have sub search set limited by this regardless PPM?
      f_search_set=to_vec(itpp_ext::matlab_range(-60000,5000,55000)); // 2*65kHz > 100kHz, overlap adjacent frequencies
  //    if (num_loop == 0) // it is not so useful
  //      num_loop = 36;
  //      f_search_set=to_vec(itpp_ext::matlab_range(-100000,5000,100000)); // align to matlab script
    }
  }

  cout << "    Search PSS at fo: " << f_search_set(0)/1e3 << " to " << f_search_set( length(f_search_set)-1 )/1e3 << " kHz" << endl;

  pss_fo_set_gen(f_search_set, pss_fo_set);

//  cout << "Search PSS fo(crect): " << f_search_set(0)/1e3 << " to " << f_search_set( length(f_search_set)-1 )/1e3 << " kHz" << endl;

  const uint16 n_fc=length(fc_search_set);

  // construct data for multiple tries
  const uint32 n_fc_multi_try=n_fc*num_try;
  vec fc_search_set_multi_try;
  fc_search_set_multi_try.set_length(n_fc_multi_try, false);
  for(uint32 i=0; i<n_fc; i++) {
    uint32 sp = i*num_try;
    uint32 ep = sp + num_try;
    fc_search_set_multi_try.set_subvector(sp, ep-1, fc_search_set(i));
  }

  // get coefficients of 6 RB filter (to improve SNR)
  // coef = fir1(46, (0.18e6*6+150e3)/sampling_rate);
  vec coef(( sizeof( chn_6RB_filter_coef )/sizeof(float) ));
  for (uint16 i=0; i<length(coef); i++) {
    coef(i) = chn_6RB_filter_coef[i];
  }

  cvec capbuf;

  lte_opencl_t lte_ocl(opencl_platform, opencl_device);

  #ifdef USE_OPENCL
  lte_ocl.setup_filter_my((string)"filter_my_kernels.cl", CAPLENGTH, filter_workitem);

//  if ( (length(f_search_set)*3)%num_loop != 0 ){
//    cerr << "length(f_search_set)*3 can not be divided by num_loop. " << (length(f_search_set)*3) << " " << num_loop << "\n";
//    ABORT(-1);
//  }
//  lte_ocl.setup_filter_mchn((string)"filter_mchn_kernels.cl", CAPLENGTH, length(f_search_set)*3/num_loop, pss_fo_set.cols(), xcorr_workitem);
  #ifdef FILTER_MCHN_SIMPLE_KERNEL
  lte_ocl.setup_filter_mchn((string)"filter_mchn_simple_kernel.cl", CAPLENGTH, length(f_search_set)*3, pss_fo_set.cols(), xcorr_workitem);
  #else
  lte_ocl.setup_filter_mchn((string)"filter_mchn_kernels.cl", CAPLENGTH, length(f_search_set)*3, pss_fo_set.cols(), xcorr_workitem);
  #endif
  #endif

  vec period_ppm;
  vec k_factor_set;

  // Calculate the threshold vector
  const uint8 thresh1_n_nines=12;
  double rx_cutoff=(6*12*15e3/2+4*15e3)/(FS_LTE/16/2);

  // for PSS correlate
  //cout << "DS_COMB_ARM override!!!" << endl;
#define DS_COMB_ARM 2
  mat xc_incoherent_collapsed_pow;
  imat xc_incoherent_collapsed_frq;
  vector <mat>  xc_incoherent_single(3);
  vector <mat>  xc_incoherent(3);
  vector <mat> xc(3);
  vec sp_incoherent;
  vec sp;

  // for SSS detection
#define THRESH2_N_SIGMA 3
  vec sss_h1_np_est_meas;
  vec sss_h2_np_est_meas;
  cvec sss_h1_nrm_est_meas;
  cvec sss_h2_nrm_est_meas;
  cvec sss_h1_ext_est_meas;
  cvec sss_h2_ext_est_meas;
  mat log_lik_nrm;
  mat log_lik_ext;

  // for time frequency grid
  // Extract time and frequency grid
  cmat tfg;
  vec tfg_timestamp;
  // Compensate for time and frequency offsets
  cmat tfg_comp;
  vec tfg_comp_timestamp;

  Real_Timer tt; // for profiling
  // Each center frequency is searched independently. Results are stored in this vector.
  vector < list<Cell> > detected_cells(n_fc);
  // Loop for each center frequency.
  for (uint32 fci=0;fci<n_fc_multi_try;fci++) {
    fc_requested=fc_search_set_multi_try(fci);
    uint32 fc_idx = fci/num_try;
    uint32 try_idx = fci - fc_idx*num_try;

    if (verbosity>=1) {
      cout << "\nExamining center frequency " << fc_requested/1e6 << " MHz ... try " << try_idx << endl;
    }

    // Fill capture buffer
    int run_out_of_data = capture_data(fc_requested,correction,save_cap,record_bin_filename,use_recorded_data,load_bin_filename,data_dir,rtlsdr_dev,hackrf_dev,bladerf_dev,dev_use,capbuf,fc_programmed, fs_programmed, false);
    if (run_out_of_data){
      fci = n_fc_multi_try; // end of loop
      continue;
    }

    capbuf = capbuf - mean(capbuf); // remove DC

    freq_correction = fc_programmed*(correction-1)/correction;
//    if (!dongle_used) { // if dongle is not used, do correction explicitly. Because if dongle is used, the correction is done when tuning dongle's frequency.
      capbuf = fshift(capbuf,-freq_correction,fs_programmed);
//    }

    // 6RB filter to improve SNR
//    tt.tic();
    #ifdef USE_OPENCL
//      tt.tic();
      lte_ocl.filter_my(capbuf); // be careful! capbuf.zeros() will slow down the xcorr part pretty much!
//      cout << "1 cost " << tt.get_time() << "s\n";
//
//      tt.tic();
//      filter_my_fft(coef, capbuf);
//      cout << "2 cost " << tt.get_time() << "s\n";
    #else
      filter_my(coef, capbuf);
    #endif
//    cout << "6RB filter cost " << tt.get_time() << "s\n";

    vec dynamic_f_search_set = f_search_set; // don't touch the original
    double xcorr_pss_time;
    sampling_ppm_f_search_set_by_pss(lte_ocl, num_loop, capbuf, pss_fo_set, sampling_carrier_twist, num_reserve, dynamic_f_search_set, period_ppm, xc, xcorr_pss_time);
    cout << "PSS XCORR  cost " << xcorr_pss_time << "s\n";

    list <Cell> peak_search_cells;
    if (!sampling_carrier_twist) {
      if ( isnan(period_ppm[0]) ) {
        if (verbosity>=2) cout << "No valid PSS is found at pre-proc phase! Please try again.\n";
        continue;
      } else {
        k_factor_set.set_length(length(period_ppm));
        k_factor_set = 1 + period_ppm*1e-6;
      }

      vec tmp_f_search(1);
      vector <mat> tmp_xc(3);
      tmp_xc[0].set_size(1, length(capbuf)-136);
      tmp_xc[1].set_size(1, length(capbuf)-136);
      tmp_xc[2].set_size(1, length(capbuf)-136);
      for (uint16 i=0; i<length(k_factor_set); i++) {

        tmp_f_search(0) = dynamic_f_search_set(i);
        tmp_xc[0].set_row(0, xc[0].get_row(i));
        tmp_xc[1].set_row(0, xc[1].get_row(i));
        tmp_xc[2].set_row(0, xc[2].get_row(i));

        // Correlate
        uint16 n_comb_xc;
        uint16 n_comb_sp;
        if (verbosity>=2) {
          cout << "  Calculating PSS correlations" << endl;
        }
//        tt.tic();
        xcorr_pss(capbuf,tmp_f_search,DS_COMB_ARM,fc_requested,fc_programmed,fs_programmed,tmp_xc,xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,xc_incoherent_single,xc_incoherent,sp_incoherent,sp,n_comb_xc,n_comb_sp,sampling_carrier_twist,(const double)k_factor_set[i]);
//        cout << "PSS post cost " << tt.get_time() << "s\n";

        // Calculate the threshold vector
        double R_th1=chi2cdf_inv(1-pow(10.0,-thresh1_n_nines),2*n_comb_xc*(2*DS_COMB_ARM+1));
        vec Z_th1=R_th1*sp_incoherent/rx_cutoff/137/n_comb_xc/(2*DS_COMB_ARM+1); // remove /2 to avoid many false alarm

        // Search for the peaks
        if (verbosity>=2) {
          cout << "  Searching for and examining correlation peaks..." << endl;
        }
//        tt.tic();
        peak_search(xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,Z_th1,tmp_f_search,fc_requested,fc_programmed,xc_incoherent_single,DS_COMB_ARM,sampling_carrier_twist,(const double)k_factor_set[i],peak_search_cells);
//        cout << "peak_search cost " << tt.get_time() << "s\n";
      }

    } else {

      // Correlate
      uint16 n_comb_xc;
      uint16 n_comb_sp;
      if (verbosity>=2) {
        cout << "  Calculating PSS correlations" << endl;
      }
//      tt.tic();
      xcorr_pss(capbuf,dynamic_f_search_set,DS_COMB_ARM,fc_requested,fc_programmed,fs_programmed,xc,xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,xc_incoherent_single,xc_incoherent,sp_incoherent,sp,n_comb_xc,n_comb_sp,sampling_carrier_twist,NAN);
//      cout << "PSS post cost " << tt.get_time() << "s\n";

      // Calculate the threshold vector
      double R_th1=chi2cdf_inv(1-pow(10.0,-thresh1_n_nines),2*n_comb_xc*(2*DS_COMB_ARM+1));
      vec Z_th1=R_th1*sp_incoherent/rx_cutoff/137/n_comb_xc/(2*DS_COMB_ARM+1); // remove /2 to avoid many false alarm

      // Search for the peaks
      if (verbosity>=2) {
        cout << "  Searching for and examining correlation peaks..." << endl;
      }
//      tt.tic();
      peak_search(xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,Z_th1,dynamic_f_search_set,fc_requested,fc_programmed,xc_incoherent_single,DS_COMB_ARM,sampling_carrier_twist,NAN,peak_search_cells);
//      cout << "peak_search cost " << tt.get_time() << "s\n";
    }

    detected_cells[fc_idx]=peak_search_cells;
    cout << "Hit  num peaks " << detected_cells[fc_idx].size()/2 << "\n";

    // Loop and check each peak
    list<Cell>::iterator iterator=detected_cells[fc_idx].begin();
    int tdd_flag = 1;
    uint16 tmp_count = 0;
    while (iterator!=detected_cells[fc_idx].end()) {
      tdd_flag = !tdd_flag;
      cout << "try peak " << tmp_count/2 << " tdd_flag " << tdd_flag << "\n";
      tmp_count++;
//      cout << tdd_flag << "\n";
//      cout << (*iterator).ind << "\n";
      // Detect SSS if possible
      (*iterator)=sss_detect((*iterator),capbuf,THRESH2_N_SIGMA,fc_requested,fc_programmed,fs_programmed,sss_h1_np_est_meas,sss_h2_np_est_meas,sss_h1_nrm_est_meas,sss_h2_nrm_est_meas,sss_h1_ext_est_meas,sss_h2_ext_est_meas,log_lik_nrm,log_lik_ext,sampling_carrier_twist,tdd_flag);
      if ((*iterator).n_id_1!=-1) {
        // Fine FOE
        (*iterator)=pss_sss_foe((*iterator),capbuf,fc_requested,fc_programmed,fs_programmed,sampling_carrier_twist,tdd_flag);
        // Extract time and frequency grid
        extract_tfg((*iterator),capbuf,fc_requested,fc_programmed,fs_programmed,tfg,tfg_timestamp,sampling_carrier_twist);
        // Create object containing all RS
        RS_DL rs_dl((*iterator).n_id_cell(),6,(*iterator).cp_type);
        // Compensate for time and frequency offsets
        (*iterator)=tfoec((*iterator),tfg,tfg_timestamp,fc_requested,fc_programmed,rs_dl,tfg_comp,tfg_comp_timestamp,sampling_carrier_twist);
        // Finally, attempt to decode the MIB
        (*iterator)=decode_mib((*iterator),tfg_comp,rs_dl);

        if ((*iterator).n_rb_dl==-1) {
          // No MIB could be successfully decoded.
          iterator=detected_cells[fc_idx].erase(iterator);
          continue;
        }

        if (verbosity>=1) {
          if (tdd_flag==0)
              cout << "  Detected a FDD cell! At freqeuncy " << fc_requested/1e6 << "MHz, try " << try_idx << endl;
          else
              cout << "  Detected a TDD cell! At freqeuncy " << fc_requested/1e6 << "MHz, try " << try_idx << endl;
          cout << "    cell ID: " << (*iterator).n_id_cell() << endl;
          cout << "     PSS ID: " << (*iterator).n_id_2 << endl;
          cout << "    RX power level: " << db10((*iterator).pss_pow) << " dB" << endl;
          cout << "    residual frequency offset: " << (*iterator).freq_superfine << " Hz" << endl;
          cout << "                     k_factor: " << (*iterator).k_factor << endl;
        }

        ++iterator;

      } else {
        iterator=detected_cells[fc_idx].erase(iterator);
      }
//      // Detect SSS if possible
//      #define THRESH2_N_SIGMA 3
//      cell_temp = (*iterator);
//      for(tdd_flag=0;tdd_flag<2;tdd_flag++)
//      {
////        tt.tic();
//        (*iterator)=sss_detect(cell_temp,capbuf,THRESH2_N_SIGMA,fc_requested,fc_programmed,fs_programmed,sss_h1_np_est_meas,sss_h2_np_est_meas,sss_h1_nrm_est_meas,sss_h2_nrm_est_meas,sss_h1_ext_est_meas,sss_h2_ext_est_meas,log_lik_nrm,log_lik_ext,sampling_carrier_twist,tdd_flag);
////        cout << "sss_detect cost " << tt.get_time() << "s\n";
//        if ((*iterator).n_id_1!=-1)
//            break;
//      }
//      if ((*iterator).n_id_1==-1) {
//        // No SSS detected.
//
//        continue;
//      }
//      // Fine FOE
////      tt.tic();
//      (*iterator)=pss_sss_foe((*iterator),capbuf,fc_requested,fc_programmed,fs_programmed,sampling_carrier_twist,tdd_flag);
////      cout << "pss_sss_foe cost " << tt.get_time() << "s\n";
//
//      // Extract time and frequency grid
////      tt.tic();
//      extract_tfg((*iterator),capbuf,fc_requested,fc_programmed,fs_programmed,tfg,tfg_timestamp,sampling_carrier_twist);
////      cout << "extract_tfg cost " << tt.get_time() << "s\n";
//
//      // Create object containing all RS
//      RS_DL rs_dl((*iterator).n_id_cell(),6,(*iterator).cp_type);
//
//      // Compensate for time and frequency offsets
////      tt.tic();
//      (*iterator)=tfoec((*iterator),tfg,tfg_timestamp,fc_requested,fc_programmed,rs_dl,tfg_comp,tfg_comp_timestamp,sampling_carrier_twist);
////      cout << "tfoec cost " << tt.get_time() << "s\n";
//
//      // Finally, attempt to decode the MIB
////      tt.tic();
//      (*iterator)=decode_mib((*iterator),tfg_comp,rs_dl);
////      cout << "decode_mib cost " << tt.get_time() << "s\n";
//      if ((*iterator).n_rb_dl==-1) {
//        // No MIB could be successfully decoded.
//        iterator=detected_cells[fc_idx].erase(iterator);
//        continue;
//      }
//
//      if (verbosity>=1) {
//        if (tdd_flag==0)
//            cout << "  Detected a FDD cell! At freqeuncy " << fc_requested/1e6 << "MHz, try " << try_idx << endl;
//        else
//            cout << "  Detected a TDD cell! At freqeuncy " << fc_requested/1e6 << "MHz, try " << try_idx << endl;
//        cout << "    cell ID: " << (*iterator).n_id_cell() << endl;
//        cout << "     PSS ID: " << (*iterator).n_id_2 << endl;
//        cout << "    RX power level: " << db10((*iterator).pss_pow) << " dB" << endl;
//        cout << "    residual frequency offset: " << (*iterator).freq_superfine << " Hz" << endl;
//        cout << "                     k_factor: " << (*iterator).k_factor << endl;
//      }
//
//      ++iterator;
    }
    if (detected_cells[fc_idx].size() > 0){
      fci = (fc_idx+1)*num_try - 1; // skip to next frequency
    }
  }

  if (dev_use == dev_type_t::HACKRF) {
    #ifdef HAVE_HACKRF
    if (hackrf_dev != NULL) {
      hackrf_close(hackrf_dev);
      hackrf_dev = NULL;
      hackrf_exit();
    }
    #endif
  }
  if (dev_use == dev_type_t::RTLSDR) {
    #ifdef HAVE_RTLSDR
    if (rtlsdr_dev != NULL) {
      rtlsdr_close(rtlsdr_dev);
      rtlsdr_dev = NULL;
    }
    #endif
  }
  if (dev_use == dev_type_t::BLADERF) {
    #ifdef HAVE_BLADERF
    if (bladerf_dev!=NULL) {
      bladerf_close(bladerf_dev);
      bladerf_dev = NULL;
    }
    #endif
  }

  // Generate final list of detected cells.
  list <Cell> cells_final;
  dedup(detected_cells,cells_final);

  // Print out the final list of detected cells.
  if (cells_final.size()==0) {
    cout << "No LTE cells were found..." << endl;
  } else {
    cout << "Detected the following cells:" << endl;
    cout << "DPX:TDD/FDD; A: #antenna ports C: CP type ; P: PHICH duration ; PR: PHICH resource type" << endl;
    cout << "DPX CID A      fc   freq-offset RXPWR C nRB P  PR CrystalCorrectionFactor" << endl;
    list <Cell>::iterator it=cells_final.begin();
    while (it!=cells_final.end()) {
      // Use a stringstream to avoid polluting the iostream settings of cout.
      stringstream ss;
      if ((*it).duplex_mode == 1)
        ss << "TDD ";
      else
        ss << "FDD ";
      ss << setw(3) << (*it).n_id_cell();
      ss << setw(2) << (*it).n_ports;
      ss << " " << setw(6) << setprecision(5) << (*it).fc_requested/1e6 << "M";
      ss << " " << setw(13) << freq_formatter((*it).freq_superfine);
      ss << " " << setw(5) << setprecision(3) << db10((*it).pss_pow);
      ss << " " << (((*it).cp_type==cp_type_t::NORMAL)?"N":(((*it).cp_type==cp_type_t::UNKNOWN)?"U":"E"));
      ss << " " << setw(3) << (*it).n_rb_dl;
      ss << " " << (((*it).phich_duration==phich_duration_t::NORMAL)?"N":(((*it).phich_duration==phich_duration_t::UNKNOWN)?"U":"E"));
      switch ((*it).phich_resource) {
        case phich_resource_t::UNKNOWN: ss << " UNK"; break;
        case phich_resource_t::oneSixth: ss << " 1/6"; break;
        case phich_resource_t::half: ss << " 1/2"; break;
        case phich_resource_t::one: ss << " one"; break;
        case phich_resource_t::two: ss << " two"; break;
      }

      // Calculate the correction factor.
      // This is where we know the carrier is located
      const double true_location=(*it).fc_programmed;
      // We can calculate the RTLSDR's actualy frequency
      const double crystal_freq_actual=(*it).fc_programmed-(*it).freq_superfine;
      // Calculate correction factors
      const double correction_residual=true_location/crystal_freq_actual;
      double correction_new=correction*correction_residual;
//      if (!sampling_carrier_twist) {
//        correction_new = (*it).k_factor;
//      }
      ss << " " << setprecision(20) << correction_new;
      cout << ss.str() << endl;

      ++it;
    }
  }

  // Successful exit.
  return 0;
}

