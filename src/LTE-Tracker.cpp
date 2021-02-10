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
#include <unistd.h>
#include <itpp/itbase.h>
#include <itpp/stat/misc_stat.h>
#include <itpp/signal/transforms.h>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <list>
#include <sstream>
#include <queue>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <curses.h>
#include <signal.h>

#include "common.h"

#ifdef HAVE_RTLSDR
#include "rtl-sdr.h"
#endif // HAVE_RTLSDR

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

#include "common.h"
#include "macros.h"
#include "lte_lib.h"
#include "constants.h"
#include "capbuf.h"
#include "itpp_ext.h"
#include "searcher.h"
#include "dsp.h"
#include "LTE-Tracker.h"
#include "filter_coef.h"

#ifdef HAVE_HACKRF
#include "hackrf.h"
#endif

using namespace itpp;
using namespace std;

uint8 verbosity=1;

// Global variables that can be set by the command line. Used for debugging.
double global_1=0;
double global_2=0;
double global_3=0;
double global_4=0;
double global_5=0;
double global_6=0;
double global_7=0;
double global_8=0;
double global_9=0;

// Simple usage screen.
void print_usage() {
  cout << "LTE-Tracker (" << BUILD_TYPE << ") help. 1.0 to " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_LEVEL << ": OpenCL/TDD/HACKRF/bladeRF/ext-LNB added by Jiao Xianjun(putaoshu@gmail.com)" << endl << endl;
  cout << "LTE-Tracker -f frequency [optional_parameters]" << endl;
  cout << "  Basic options" << endl;
  cout << "    -h --help" << endl;
  cout << "      print this help screen" << endl;
  cout << "    -v --verbose" << endl;
  cout << "      increase status messages from program" << endl;
  cout << "    -b --brief" << endl;
  cout << "      reduce status messages from program" << endl;
  cout << "    -i --device-index N" << endl;
  cout << "      specify which attached RTLSDR dongle to use" << endl;
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
  cout << "    -r --repeat" << endl;
  cout << "      cyclically repeat the data read from the file forever" << endl;
  cout << "  Frequency options:" << endl;
  cout << "    -f --freq fc" << endl;
  cout << "      frequency where cells are located" << endl;
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
  // Hidden option...
  //cout << "    -x --expert" << endl;
  //cout << "      enable expert mode display" << endl;
  // Hidden options. Only useful for debugging.
  //cout << "  Capture buffer options:" << endl;
//  cout << "    -z --recbin" << endl;
//  cout << "      save captured data in the bin file" << endl;
//  cout << "    -y --loadbin" << endl;
//  cout << "      used data in captured bin file" << endl;
  //cout << "    -l --load filename" << endl;
  //cout << "      read data from file instead of using live data" << endl;
  //cout << "    -r --repeat" << endl;
  //cout << "      cyclically repeat the data read from the file forever" << endl;
  //cout << "    -d --drop n" << endl;
  //cout << "      drop the first 'n' seconds of the datafile to allow for AGC convergence" << endl;
  //cout << "    -s --rtl_sdr" << endl;
  //cout << "      data file was created by the rtl_sdr program. Defaults to an itpp file." << endl;
  //cout << "    -n --noise-power value" << endl;
  //cout << "      add AWGN noise at the specified power level (in dB)
  //cout << "  Global variables used for testing" << endl;
  //cout << "    -1 --g1 value" << endl;
  //cout << "    -2 --g2 value" << endl;
  //cout << "    -3 --g3 value" << endl;
  //cout << "    -4 --g4 value" << endl;
  //cout << "    -5 --g5 value" << endl;
  //cout << "    -6 --g6 value" << endl;
  //cout << "    -7 --g7 value" << endl;
  //cout << "    -8 --g8 value" << endl;
  //cout << "    -9 --g9 value" << endl;
  cout << "See CellSearch for documentation on c and ppm." << endl;
}

// Parse the command line arguments and return optional parameters as
// variables.
// Also performs some basic sanity checks on the parameters.
void parse_commandline(
  // Inputs
  const int & argc,
  char * const argv[],
  // Outputs
  double & fc,
  double & ppm,
  double & correction,
  int & device_index,
  bool & expert_mode,
  bool & use_recorded_data,
  string & filename,
  bool & repeat,
  double & drop_secs,
  bool & rtl_sdr_format,
  double & noise_power,
  bool & sampling_carrier_twist,
  char * record_bin_filename,
  char * load_bin_filename,
  uint16 & opencl_platform,
  uint16 & opencl_device,
  uint16 & filter_workitem,
  uint16 & xcorr_workitem,
  uint16 & num_reserve,
  int16  & gain
) {
  // Default values
  fc=-1;
  sampling_carrier_twist=false;
  ppm=120;
  correction=1;
  device_index=-1;
  expert_mode=false;
  use_recorded_data=false;
  repeat=true;
  drop_secs=0;
  rtl_sdr_format=false;
  noise_power=0;
  opencl_platform = 0;
  opencl_device = 0;
  filter_workitem = 32;
  xcorr_workitem = 2;
  num_reserve = 2;
  gain = -9999;

  while (1) {
    static struct option long_options[] = {
      {"help",         no_argument,       0, 'h'},
      {"verbose",      no_argument,       0, 'v'},
      {"brief",        no_argument,       0, 'b'},
      {"freq",         required_argument, 0, 'f'},
      {"num-reserve",         required_argument, 0, 'm'},
      {"twisted",      no_argument,       0, 't'},
      {"ppm",          required_argument, 0, 'p'},
      {"correction",   required_argument, 0, 'c'},
      {"device-index", required_argument, 0, 'i'},
      {"opencl-platform", required_argument, 0, 'a'},
      {"gain", required_argument, 0, 'g'},
      {"opencl-device", required_argument, 0, 'j'},
      {"filter-workitem", required_argument, 0, 'w'},
      {"xcorr-workitem", required_argument, 0, 'u'},
      {"expert",       no_argument,       0, 'x'},
      {"recbin",       required_argument, 0, 'z'},
      {"loadbin",      required_argument, 0, 'y'},
      {"load",         required_argument, 0, 'l'},
      {"repeat",       no_argument,       0, 'r'},
      {"drop",         required_argument, 0, 'd'},
      {"rtl_sdr",      no_argument,       0, 's'},
      {"noise-power",  required_argument, 0, 'n'},
      {"g1",           required_argument, 0, '1'},
      {"g2",           required_argument, 0, '2'},
      {"g3",           required_argument, 0, '3'},
      {"g4",           required_argument, 0, '4'},
      {"g5",           required_argument, 0, '5'},
      {"g6",           required_argument, 0, '6'},
      {"g7",           required_argument, 0, '7'},
      {"g8",           required_argument, 0, '8'},
      {"g9",           required_argument, 0, '9'},
      {0, 0, 0, 0}
    };
    /* getopt_long stores the option index here. */
    int option_index = 0;
    int c = getopt_long (argc, argv, "hvbf:m:tp:c:i:a:g:j:w:u:xz:y:l:rd:sn:1:2:3:4:5:6:7:8:9:",
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
      case 'f':
        fc=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse frequency" << endl;
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
      case 'x':
        expert_mode=true;
        break;
      case 'l':
        use_recorded_data=true;
        filename=optarg;
        break;
      case 'r':
        repeat=true;
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
        drop_secs=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse drop value" << endl;
          ABORT(-1);
        }
        break;
      case 's':
        rtl_sdr_format=true;
        break;
      case 'n':
        noise_power=strtod(optarg,&endp);
        noise_power=udb10(noise_power);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse noise power" << endl;
          ABORT(-1);
        }
        break;
      case '1':
        global_1=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 1" << endl;
          ABORT(-1);
        }
        break;
      case '2':
        global_2=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 2" << endl;
          ABORT(-1);
        }
        break;
      case '3':
        global_3=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 3" << endl;
          ABORT(-1);
        }
        break;
      case '4':
        global_4=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 4" << endl;
          ABORT(-1);
        }
        break;
      case '5':
        global_5=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 5" << endl;
          ABORT(-1);
        }
        break;
      case '6':
        global_6=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 6" << endl;
          ABORT(-1);
        }
        break;
      case '7':
        global_7=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 7" << endl;
          ABORT(-1);
        }
        break;
      case '8':
        global_8=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 8" << endl;
          ABORT(-1);
        }
        break;
      case '9':
        global_9=strtod(optarg,&endp);
        if ((optarg==endp)||(*endp!='\0')) {
          cerr << "Error: could not parse global variable 9" << endl;
          ABORT(-1);
        }
        break;
      case '?':
        /* getopt_long already printed an error message. */
        ABORT(-1);
      default:
        ABORT(-1);
    }
  }

  // Error if extra arguments are found on the command line
  if (optind<argc) {
    cerr << "Error: unknown/extra arguments specified on command line" << endl;
    ABORT(-1);
  }

  // Second order command line checking. Ensure that command line options
  // are consistent.
  if (fc==-1) {
//    if (!sampling_carrier_twist) {
      fc=9999e6; // fake
      cout << "Warning: Frequency not specified. Make sure you are working on captured file.\n";
//    } else {
//      cerr << "Error: must specify a start frequency. (Try --help)" << endl;
//      ABORT(-1);
//    }
  }
  // Start and end frequencies should be on a 100kHz raster.
  if (fc<1e6) {
    cerr << "Error: frequency must be greater than 1MHz" << endl;
    ABORT(-1);
  }
//  if (fc/100e3!=itpp::round(fc/100e3)) {
//    fc=itpp::round(fc/100e3)*100e3;
//    cout << "Warning: frequency has been rounded to the nearest multiple of 100kHz" << endl;
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
  // Drop and repeat will probably not be used at the same time.
  if ((drop_secs!=0)&&(repeat)) {
    cout << "Warning: --drop and --repeat were both requested" << endl;
  }
// Should never both read and write captured data from a file
//  if (save_cap&&use_recorded_data) {
//    cerr << "Error: cannot read and write captured data at the same time!" << endl;
//    ABORT(-1);
//  }
  // Should never both read from .it and read from .bin file.
  if ( use_recorded_data && (strlen(load_bin_filename)>4) ) {
    cerr << "Error: cannot read from .it and .bin file at the same time!" << endl;
    ABORT(-1);
  }

  if (verbosity>=1) {
    cout << "OpenCL LTE Tracker (" << BUILD_TYPE << ") beginning. 1.0 to " << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_LEVEL << ": OpenCL/TDD/HACKRF/bladeRF/ext-LNB added by Jiao Xianjun(putaoshu@gmail.com)" << endl;
//    cout << "  Search frequency: " << fc/1e6 << " MHz" << endl;
//    if (sampling_carrier_twist) {
      cout << "  PPM: " << ppm << endl;
      stringstream temp;
      temp << setprecision(20) << correction;
      cout << "  correction: " << temp.str() << endl;
//    }
//    if (save_cap)
//      cout << "  Captured data will be saved in capbufXXXX.it files" << endl;
    if (use_recorded_data)
      cout << "  Captured data will be read from capbufXXXX.it files" << endl;
  }
}

// In high SNR environments, a cell may be detected on different carrier
// frequencies and with different frequency offsets. Keep only the cell
// with the highest received power.
void dedup(
  const list <Cell> & detected_cells,
  list <Cell> & cells_final
) {
  cells_final.clear();
  list <Cell>::const_iterator it_n=detected_cells.begin();
  while (it_n!=detected_cells.end()) {
    bool match=false;
    list <Cell>::iterator it_f=cells_final.begin();
    while (it_f!=cells_final.end()) {
      // Do these detected cells match?
      if ((*it_n).n_id_cell()==(*it_f).n_id_cell()) {
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

/*
static void rtlsdr_callback_temp(unsigned char *buf, uint32 len, void *ctx) {
  if (ctx) {

    if (fwrite(buf, 1, len, (FILE*)ctx) != len) {
      fprintf(stderr, "Short write, samples lost, exiting!\n");
      //rtlsdr_cancel_async(rtlsdr_dev);
    }
  }
}
*/

#ifdef HAVE_RTLSDR
// Open the RTLSDR device
int config_rtlsdr(
  const bool & sampling_carrier_twist,
  const double & correction,
  const int32 & device_index_cmdline,
  const double & fc,
  rtlsdr_device *& dev,
  double & fs_programmed,
  const int16 gain
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
  hackrf_device *& dev,
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

  int gian_set = ( gain==-9999?66:gain );
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

// Read a file either in rtlsdr format or itpp format.
void read_datafile(
  const string & filename,
  const bool & rtl_sdr_format,
  const double & drop_secs,
  cvec & sig_tx
) {
  if (!rtl_sdr_format) {
    it_ifile itf(filename);
    itf.seek("sig_tx");
    itf>>sig_tx;
  } else {
    itpp_ext::rtl_sdr_to_cvec(filename,sig_tx);
  }
  // Drop several seconds while AGC converges.
  sig_tx=sig_tx(MIN(round_i(FS_LTE/16*drop_secs),length(sig_tx)-1),length(sig_tx)-1);
  if (length(sig_tx)==0) {
    cerr << "Error: not enough data in file!" << endl;
    ABORT(-1);
  }
}

// Perform an initial cell search solely for the purpose of calibrating
// the oscillator.
// This code can probably be rolled into the searcher so as to eliminate
// some duplicated code.
double kalibrate(
  double & fc_requested,
  double & fs_programmed,
  const double & ppm,
  const double & correction,
  double & correction_new,
  const bool & use_recorded_data,
  const string & filename,
  const bool & rtl_sdr_format,
  const double & noise_power,
  const double & drop_secs,
  const bool & repeat,
  rtlsdr_device * & rtlsdr_dev,
  hackrf_device * & hackrf_dev,
  bladerf_device * & bladerf_dev,
  const dev_type_t::dev_type_t & dev_use,
  double & fc_programmed,
  const bool & sampling_carrier_twist,
  double & k_factor,
  const char * record_bin_filename,
  const char * load_bin_filename,
  const uint16 & opencl_platform,
  const uint16 & opencl_device,
  const uint16 & filter_workitem,
  const uint16 & xcorr_workitem,
  const uint16 & num_reserve
) {
  if (verbosity>=1) {
    cout << "Calibrating local oscillator." << endl;
  }

  string data_dir=".";

  bool dongle_used = (!use_recorded_data) && (strlen(load_bin_filename)==0);

  double fc_requested_tmp, fc_programmed_tmp, fs_requested_tmp, fs_programmed_tmp;
  if ( dongle_used && fc_requested!=9999e6) {
    if (dev_use == dev_type_t::RTLSDR) {
      #ifdef HAVE_RTLSDR
      fc_programmed_tmp = calculate_fc_programmed_in_context(fc_requested, use_recorded_data, load_bin_filename, rtlsdr_dev);
      #endif
    } else  if (dev_use == dev_type_t::HACKRF || dev_use == dev_type_t::BLADERF) {
      fc_programmed_tmp = fc_requested;
    }

    cout << "Use dongle begin with " << ( fc_requested/1e6 ) << "MHz actual " << (fc_programmed_tmp/1e6) << "MHz " << fs_programmed << "MHz\n";
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

    if (fc_requested==9999e6)
      fc_requested = fc_requested_tmp;

    cout << "Use file begin with " << ( fc_requested_tmp/1e6 ) << "MHz actual " << (fc_programmed_tmp/1e6) << "MHz " << fs_programmed << "MHz\n";
  }

  vec fc_search_set(1);
  fc_search_set(0) = fc_requested;

  double freq_correction = fc_programmed_tmp*(correction-1)/correction;

  cout << "    Search frequency: " << fc_search_set(0)/1e6 << " to " <<  fc_search_set( length(fc_search_set)-1 )/1e6 << " MHz" << endl;
  cout << "with freq correction: " << freq_correction/1e3 << " kHz" << endl;

  // Generate a list of frequency offsets that should be searched for each
  // center frequency.
  cmat pss_fo_set;// pre-generate frequencies offseted pss time domain sequence
  vec f_search_set;
  if (sampling_carrier_twist) { // original mode
    const uint16 n_extra=floor_i((fc_search_set(0)*ppm/1e6+2.5e3)/5e3);
    f_search_set=to_vec(itpp_ext::matlab_range( -n_extra*5000,5000, (n_extra-1)*5000));
  } else {
    if (length(fc_search_set)==1) {//when only one frequency is specified, whole PPM range should be covered
      const uint16 n_extra=floor_i((fc_search_set(0)*ppm/1e6+2.5e3)/5e3);
      f_search_set=to_vec(itpp_ext::matlab_range( -n_extra*5000,5000, (n_extra-1)*5000));
    } else {
      // since we have frequency step is 100e3, why not have sub search set limited by this regardless PPM?
      f_search_set=to_vec(itpp_ext::matlab_range(-60000,5000,55000)); // 2*65kHz > 100kHz, overlap adjacent frequencies
    }
  }

  cout << "    Search PSS at fo: " << f_search_set(0)/1e3 << " to " << f_search_set( length(f_search_set)-1 )/1e3 << " kHz" << endl;

  pss_fo_set_gen(f_search_set, pss_fo_set);

  vec coef(( sizeof( chn_6RB_filter_coef )/sizeof(float) ));
  for (uint16 i=0; i<length(coef); i++) {
    coef(i) = chn_6RB_filter_coef[i];
  }

  cvec capbuf;
  lte_opencl_t lte_ocl(opencl_platform, opencl_device);

  #ifdef USE_OPENCL
  lte_ocl.setup_filter_my((string)"filter_my_kernels.cl", CAPLENGTH, filter_workitem);
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
  vector <mat> xc_incoherent_single(3);
  vector <mat> xc_incoherent(3);
  vec sp_incoherent;
  vector <mat> xc(3);
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

  // Results are stored in this vector.
  list <Cell> detected_cells;
  // Loop until a cell is found
  while (detected_cells.size()<1) {
   // Fill capture buffer either from a file or from live data.
//    if (use_recorded_data) {
//      capbuf.set_size(153600);
//      cvec cbload;
//      read_datafile(filename,rtl_sdr_format,drop_secs,cbload);
//      if (length(cbload)>=length(capbuf)) {
//        capbuf=cbload(0,length(capbuf)-1);
//      } else {
//        if (!repeat) {
//          cerr << "Error: not enough data in file!" << endl;
//          ABORT(-1);
//        }
//        for (int32 t=0;t<length(capbuf);t++) {
//          capbuf(t)=cbload(mod(t,length(cbload)));
//        }
//      }
//      fc_programmed=fc_requested;
//    } else {
//      capture_data(fc_requested,1.0,false,"no",false,"no",".",rtlsdr_dev,capbuf,fc_programmed);
//    }
    int run_out_of_data = capture_data(fc_requested,correction,false,record_bin_filename,use_recorded_data,load_bin_filename,".",rtlsdr_dev,hackrf_dev, bladerf_dev, dev_use,capbuf,fc_programmed,fs_programmed,false);
    if (run_out_of_data){
      cerr << "Run out of data.\n";
      ABORT(-1);
    }

    capbuf = capbuf - mean(capbuf); // remove DC

    freq_correction = fc_programmed*(correction-1)/correction;
    capbuf = fshift(capbuf,-freq_correction,fs_programmed);

    //cout << "Capbuf power: " << db10(sigpower(capbuf)) << " dB" << endl;
    if (noise_power)
      capbuf+=blnoise(length(capbuf))*sqrt(noise_power);

    // 6RB filter to improve SNR
//    tt.tic();
    #ifdef USE_OPENCL
      lte_ocl.filter_my(capbuf); // be careful! capbuf.zeros() will slow down the xcorr part pretty much!
    #else
      filter_my(coef, capbuf);
    #endif
//    cout << "6RB filter cost " << tt.get_time() << "s\n";

    vec dynamic_f_search_set = f_search_set; // don't touch the original
    double xcorr_pss_time;
    sampling_ppm_f_search_set_by_pss(lte_ocl, 0, capbuf, pss_fo_set, sampling_carrier_twist, num_reserve, dynamic_f_search_set, period_ppm, xc,xcorr_pss_time);
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
        xcorr_pss(capbuf,tmp_f_search,DS_COMB_ARM,fc_requested,fc_programmed,fs_programmed,tmp_xc,xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,xc_incoherent_single,xc_incoherent,sp_incoherent,sp,n_comb_xc,n_comb_sp,sampling_carrier_twist,(const double)k_factor_set[i]);

        // Calculate the threshold vector
        double R_th1=chi2cdf_inv(1-pow(10.0,-thresh1_n_nines),2*n_comb_xc*(2*DS_COMB_ARM+1));
        vec Z_th1=R_th1*sp_incoherent/rx_cutoff/137/n_comb_xc/(2*DS_COMB_ARM+1);

        // Search for the peaks
        if (verbosity>=2) {
          cout << "  Searching for and examining correlation peaks..." << endl;
        }
        peak_search(xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,Z_th1,tmp_f_search,fc_requested,fc_programmed,xc_incoherent_single,DS_COMB_ARM,sampling_carrier_twist,(const double)k_factor_set[i],peak_search_cells);

      }

    } else {

      // Correlate
      uint16 n_comb_xc;
      uint16 n_comb_sp;
      if (verbosity>=2) {
        cout << "  Calculating PSS correlations" << endl;
      }
      xcorr_pss(capbuf,dynamic_f_search_set,DS_COMB_ARM,fc_requested,fc_programmed,fs_programmed,xc,xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,xc_incoherent_single,xc_incoherent,sp_incoherent,sp,n_comb_xc,n_comb_sp,sampling_carrier_twist,NAN);

      // Calculate the threshold vector
      double R_th1=chi2cdf_inv(1-pow(10.0,-thresh1_n_nines),2*n_comb_xc*(2*DS_COMB_ARM+1));
      vec Z_th1=R_th1*sp_incoherent/rx_cutoff/137/n_comb_xc/(2*DS_COMB_ARM+1);

      // Search for the peaks
      if (verbosity>=2) {
        cout << "  Searching for and examining correlation peaks..." << endl;
      }
      peak_search(xc_incoherent_collapsed_pow,xc_incoherent_collapsed_frq,Z_th1,dynamic_f_search_set,fc_requested,fc_programmed,xc_incoherent_single,DS_COMB_ARM,sampling_carrier_twist,NAN,peak_search_cells);

    }
    detected_cells=peak_search_cells;

    // Loop and check each peak
    list<Cell>::iterator iterator=detected_cells.begin();
    int tdd_flag = 1;
    while (iterator!=detected_cells.end()) {
      tdd_flag = !tdd_flag;

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
          iterator=detected_cells.erase(iterator);
          continue;
        }

        if (verbosity>=2) {
          if (tdd_flag==0)
              cout << "  Detected a FDD cell!" << endl;
          else
              cout << "  Detected a TDD cell!" << endl;
          cout << "    cell ID: " << (*iterator).n_id_cell() << endl;
          cout << "     PSS ID: " << (*iterator).n_id_2 << endl;
          cout << "    RX power level: " << db10((*iterator).pss_pow) << " dB" << endl;
          cout << "    residual frequency offset: " << (*iterator).freq_superfine << " Hz" << endl;
          cout << "                     k_factor: " << (*iterator).k_factor << endl;
        }

        ++iterator;
      } else {
        iterator=detected_cells.erase(iterator);
      }
    }

    if ((detected_cells.size()==0)&&(verbosity>=1)) {
      cout << "Calibration failed (no cells detected). Trying again..." << endl;
    }
  }

  // Generate final list of detected cells. (Remove duplicates).
  list <Cell> cells_final;
  dedup(detected_cells,cells_final);

  // Search for the strongest cell. It should always be cells_final[0], but
  // perform a search just in case...
  Cell best;
  best.pss_pow=-INFINITY;
  list <Cell>::iterator it=cells_final.begin();
  while (it!=cells_final.end()) {
    if ((*it).pss_pow>best.pss_pow) {
      best=(*it);
    }
    ++it;
  }

  // Calculate the correction factor.
  // This is where we know the carrier is located
//  const double true_location=fc_requested;
  const double true_location=fc_programmed;
  // We can calculate the RTLSDR's actual frequency
  const double crystal_freq_actual=fc_programmed-best.freq_superfine;
  // Calculate correction factors
  double correction_residual=true_location/crystal_freq_actual;
  correction_new=correction*correction_residual;

//  if (!sampling_carrier_twist) {
//    correction_residual = best.k_factor;
//  }
    k_factor = best.k_factor;
  if (verbosity>=1) {
    cout << "Calibration succeeded!" << endl;
    cout << "   Residual frequency offset: " << best.freq_superfine << " Hz" << endl;
    cout << "   New correction factor: ";
    stringstream ss;
    ss << setprecision(20) << correction_new;
    cout << ss.str() << endl;
  }

  return best.freq_superfine;
}

sampbuf_sync_t sampbuf_sync;

#ifdef HAVE_HACKRF

//static int hackrf_callback(hackrf_transfer* transfer)
// {
////  cout << "1\n";
//  if (transfer->valid_length==0) {
//    cerr << "Error: received no samples from HACKRF device..." << endl;
//    ABORT(-1);
//  }
//
//  boost::mutex::scoped_lock lock(sampbuf_sync.mutex);
//  for (uint32 t=0;t<(uint32)transfer->valid_length;t++) {
//    sampbuf_sync.fifo.push_back((int8)transfer->buffer[t]);
//  }
//  sampbuf_sync.fifo_peak_size=MAX(sampbuf_sync.fifo.size(),sampbuf_sync.fifo_peak_size);
//  sampbuf_sync.condition.notify_one();
//
//  return(0);
//}

#endif

#ifdef HAVE_RTLSDR
static void rtlsdr_callback(
  unsigned char * buf,
  uint32_t len,
  void * ctx
) {
  sampbuf_sync_t & sampbuf_sync=*((sampbuf_sync_t *)ctx);

  //cout << "Callback with " << len << " samples" << endl;

  if (len==0) {
    cerr << "Error: received no samples from USB device..." << endl;
    ABORT(-1);
  }

  boost::mutex::scoped_lock lock(sampbuf_sync.mutex);
  for (uint32 t=0;t<len;t++) {
    sampbuf_sync.fifo.push_back(buf[t]-128);
  }
  sampbuf_sync.fifo_peak_size=MAX(sampbuf_sync.fifo.size(),sampbuf_sync.fifo_peak_size);
  sampbuf_sync.condition.notify_one();
}
#endif

// Main routine.
int main(
  const int argc,
  char * const argv[]
) {
  // Command line parameters are stored here.
  double fc_requested;
  double ppm;
  double correction;
  int32 device_index;
  bool expert_mode;
  bool use_recorded_data;
  string filename;
  bool repeat;
  double drop_secs;
  bool rtl_sdr_format;
  double noise_power;
  bool initial_sampling_carrier_twist;
  char record_bin_filename[256] = {0};
  char load_bin_filename[256] = {0};
  uint16 opencl_platform;
  uint16 opencl_device;
  uint16 filter_workitem;
  uint16 xcorr_workitem;
  uint16 num_reserve;
  int16 gain;
  // Get search parameters from the user
  parse_commandline(argc,argv,fc_requested,ppm,correction,device_index,expert_mode,use_recorded_data,filename,repeat,drop_secs,rtl_sdr_format,noise_power,initial_sampling_carrier_twist,record_bin_filename,load_bin_filename,opencl_platform,opencl_device,filter_workitem,xcorr_workitem,num_reserve,gain);

  // Open the USB device.
  dev_type_t::dev_type_t dev_use = dev_type_t::UNKNOWN;
  rtlsdr_device * rtlsdr_dev=NULL;
  hackrf_device * hackrf_dev = NULL;
  bladerf_device * bladerf_dev = NULL;

  double fs_programmed;
  if ( (!use_recorded_data) && (strlen(load_bin_filename)==0) ) {

    #ifdef HAVE_RTLSDR
    if ( config_rtlsdr(initial_sampling_carrier_twist,correction,device_index,fc_requested,rtlsdr_dev,fs_programmed,gain) == 0 ) {
      dev_use = dev_type_t::RTLSDR;
      cout << "RTLSDR device FOUND!\n";
    } else {
      cout << "RTLSDR device not FOUND!\n";
      ABORT(-1);
    }
    #endif

    #ifdef HAVE_HACKRF
    if ( config_hackrf(initial_sampling_carrier_twist,correction,device_index,fc_requested,hackrf_dev,fs_programmed,gain) == 0 ) {
      dev_use = dev_type_t::HACKRF;
      cout << "HACKRF device FOUND!\n";
    } else {
      cout << "HACKRF device not FOUND!\n";
      ABORT(-1);
    }
    #endif

    #ifdef HAVE_BLADERF
    if ( config_bladerf(initial_sampling_carrier_twist,correction,device_index,fc_requested,bladerf_dev,fs_programmed,gain) == 0 ) {
      dev_use = dev_type_t::BLADERF;
      cout << "BLADERF device FOUND!\n";
    } else {
      cout << "BLADERF device not FOUND!\n";
      ABORT(-1);
    }
    #endif
  } else {
    fs_programmed=correction*(FS_LTE/16); // will be updated in kalibrate.
  }

  // Calibrate the dongle's oscillator. This is similar to running the
  // program CellSearch with only one center frequency. All information
  // is discarded except for the frequency offset.
  double fc_programmed, correction_new;
  double initial_k_factor = 1.0;
//  cout << fc_requested << " " << fs_programmed << " " << ppm << " " << correction << " " << correction_new << " " << use_recorded_data << " " << filename << " " << rtl_sdr_format << " "  << noise_power << " "  << drop_secs << " " <<  repeat << "\n";
//  cout << rtlsdr_dev << " " << hackrf_dev << " " << bladerf_dev << " " <<  dev_use << " " <<  fc_programmed << " " <<  initial_sampling_carrier_twist << " " <<  initial_k_factor << " " <<  record_bin_filename << " " <<  load_bin_filename << " " <<  opencl_platform << " " <<  opencl_device << " " <<  filter_workitem << " " <<  xcorr_workitem << " " <<  num_reserve << "\n";
  double initial_freq_offset=kalibrate(fc_requested,fs_programmed,ppm,correction,correction_new,use_recorded_data,filename,rtl_sdr_format,noise_power,drop_secs,repeat,rtlsdr_dev,hackrf_dev,bladerf_dev,dev_use,fc_programmed,initial_sampling_carrier_twist,initial_k_factor,record_bin_filename,load_bin_filename,opencl_platform,opencl_device,filter_workitem,xcorr_workitem,num_reserve);
//  // ---------------- stop and close hackrf
//  Real_Timer tt;
//  tt.tic();
//  while(tt.get_time()<2) {;}
//  if ( dev_use == dev_type_t::HACKRF ) {
//  #ifdef HAVE_HACKRF
//    int result = hackrf_stop_rx(hackrf_dev);
//    if( result != HACKRF_SUCCESS ) {
//      printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//      ABORT(-1);
//    }
//    result = hackrf_close(hackrf_dev);
//    if( result != HACKRF_SUCCESS ) {
//      printf("hackrf_close() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//      ABORT(-1);
//    }
//    result = hackrf_exit();
//    if( result != HACKRF_SUCCESS ) {
//      printf("hackrf_exit() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//      ABORT(-1);
//    }
//  #endif
//  }

  // Data shared between threads
//  sampbuf_sync_t sampbuf_sync;
  tracked_cell_list_t tracked_cell_list;
  capbuf_sync_t capbuf_sync;
  global_thread_data_t global_thread_data(fc_requested,fc_programmed,fs_programmed);
  /*
  cout << "fc_requested = " << fc_requested << endl;
  cout << "fc_programmed = " << fc_programmed << endl;
  cout << "fc_requested-fc_programmed = " << fc_requested-fc_programmed << endl;
  cout << "fs_programmed = " << fs_programmed << endl;
  cout << "fs_programmed-1.92e6 = " << fs_programmed-1.92e6 << endl;
  */
  global_thread_data.main_thread_id=syscall(SYS_gettid);

  global_thread_data.frequency_offset(initial_freq_offset);
  global_thread_data.initial_frequency_offset(initial_freq_offset);

  global_thread_data.dev_use(dev_use);

  global_thread_data.k_factor(initial_k_factor);
  global_thread_data.correction(correction_new);
  global_thread_data.sampling_carrier_twist(initial_sampling_carrier_twist);
  global_thread_data.filter_workitem(filter_workitem);
  global_thread_data.xcorr_workitem(filter_workitem/4);
  global_thread_data.opencl_platform(opencl_platform);
  global_thread_data.opencl_device(opencl_device);

  global_thread_data.rtlsdr_dev(rtlsdr_dev);
  global_thread_data.hackrf_dev(hackrf_dev);
  global_thread_data.bladerf_dev(bladerf_dev);

  // Start the cell searcher thread.
  // Now that the oscillator has been calibrated, we can perform
  // a 'real' search.
  capbuf_sync.request=false;
  capbuf_sync.capbuf.set_size(19200*8);
  boost::thread searcher_thr(searcher_thread,boost::ref(capbuf_sync),boost::ref(global_thread_data),boost::ref(tracked_cell_list));

  // Start the producer thread.
  boost::thread producer_thr(producer_thread,boost::ref(sampbuf_sync),boost::ref(capbuf_sync),boost::ref(global_thread_data),boost::ref(tracked_cell_list),boost::ref(fc_programmed));

  sampbuf_sync.fifo_peak_size=0;

  // Launch the display thread
  boost::thread display_thr(display_thread,boost::ref(sampbuf_sync),boost::ref(global_thread_data),boost::ref(tracked_cell_list),boost::ref(expert_mode));

  // The remainder of this thread simply copies data received from the USB
  // device (or a file!) to the producer thread. This can be considered
  // the pre_producer thread.
//  bool record_bin_flag = (strlen(record_bin_filename)>4);
  bool load_bin_flag = (strlen(load_bin_filename)>4);
  if (use_recorded_data || load_bin_flag) {
    cvec file_data;
//    read_datafile(filename,rtl_sdr_format,drop_secs,file_data);
//    //cout << db10(sigpower(file_data)) << endl;
    capture_data(fc_requested,correction,false,record_bin_filename,use_recorded_data,load_bin_filename,".",rtlsdr_dev,hackrf_dev, bladerf_dev, dev_use,file_data,fc_programmed,fs_programmed,true);

    uint32 offset=0;
    while (true) {
      {
        boost::mutex::scoped_lock lock(sampbuf_sync.mutex);
//        for (uint32 t=0;t<192000;t++) {
        for (uint32 t=0;t<(uint32)( file_data.length() );t++) {
          complex <double> samp=file_data[offset]+randn_c()*sqrt(noise_power);
//          uint8 samp_real=RAIL(round_i(real(samp)*128.0+128.0),0,255); // 127 should be 128?
//          uint8 samp_imag=RAIL(round_i(imag(samp)*128.0+128.0),0,255); // 127 should be 128?
          int8 samp_real=round_i(real(samp)*128.0); // 127 should be 128?
          int8 samp_imag=round_i(imag(samp)*128.0); // 127 should be 128?
          sampbuf_sync.fifo.push_back(samp_real);
          sampbuf_sync.fifo.push_back(samp_imag);
          offset++;
          if (offset==(unsigned)file_data.length()) {
            if (!repeat) {
//              cout << "1\n";
              break;
            }
            offset=0;
          }
        }
        sampbuf_sync.condition.notify_one();
      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      if (!repeat&&(offset==(unsigned)file_data.length())) {
        break;
      }
    }

    // Wait a few seconds before exiting.
    boost::this_thread::sleep(boost::posix_time::seconds(10));
    ABORT(-1);

  } else {

    if (dev_use == dev_type_t::RTLSDR) {
      #ifdef HAVE_RTLSDR
      // Start the async read process. This should never return.
      rtlsdr_read_async(rtlsdr_dev,rtlsdr_callback,(void *)&sampbuf_sync,0,0);
      #endif // HAVE_RTLSDR
    } else if (dev_use == dev_type_t::HACKRF) {
      #ifdef HAVE_HACKRF
      cvec capbuf;
      while(1) {
        capture_data(fc_requested,correction,false,record_bin_filename,use_recorded_data,load_bin_filename,".",rtlsdr_dev,hackrf_dev, bladerf_dev,dev_use,capbuf,fc_programmed,fs_programmed,false);
//        cout << "cap\n";
        boost::mutex::scoped_lock lock(sampbuf_sync.mutex);
        for (uint32 t=0;t<(uint32)length(capbuf);t++) {
          sampbuf_sync.fifo.push_back( (int8)(round_i( real( capbuf[t] )*128) ) );
          sampbuf_sync.fifo.push_back( (int8)(round_i( imag( capbuf[t] )*128) ) );
        }
        sampbuf_sync.fifo_peak_size=MAX(sampbuf_sync.fifo.size(),sampbuf_sync.fifo_peak_size);
        sampbuf_sync.condition.notify_one();
      }
//      #ifdef HAVE_HACKRF
//
//      tt.tic();
//      while(tt.get_time()<10) {;}
//      if ( config_hackrf(initial_sampling_carrier_twist,correction,device_index,fc_requested,hackrf_dev,fs_programmed) != 0 ) {
//        cout << "HACKRF re-config failed!\n";
//      }
//      int result;
//      result = hackrf_stop_rx(hackrf_dev);
//      if( result != HACKRF_SUCCESS ) {
//        printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//        ABORT(-1);
//      }
//
//      result = hackrf_start_rx(hackrf_dev, hackrf_callback, NULL);
//
//      if( result != HACKRF_SUCCESS ) {
//        printf("hackrf_start_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//        ABORT(-1);
//      }
//
////      while(1);
//      int tmp_count=0;
//
//      result = hackrf_is_streaming(hackrf_dev);
//      printf("hackrf_is_streaming() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//
//      while(hackrf_is_streaming(hackrf_dev) == HACKRF_TRUE) {
//        cout << (++tmp_count) << "\n";
//      }
//
//      cout << "HACKRF streaming exit abnormally!\n";
//      ABORT(-1);

//      #else
//
//      cout << "HACKRF can't be used when lib is not included!\n";
//      ABORT(-1);
//
//      #endif
      #endif // HAVE_HACKRF

    } else if (dev_use == dev_type_t::BLADERF) {
      #ifdef HAVE_BLADERF
      cvec capbuf;
      while(1) {
        capture_data(fc_requested,correction,false,record_bin_filename,use_recorded_data,load_bin_filename,".",rtlsdr_dev,hackrf_dev, bladerf_dev, dev_use,capbuf,fc_programmed,fs_programmed,false);
//        cout << "cap\n";
        boost::mutex::scoped_lock lock(sampbuf_sync.mutex);
        for (uint32 t=0;t<(uint32)length(capbuf);t++) {
          sampbuf_sync.fifo.push_back( (int8)(round_i( real( capbuf[t] )*128) ) );
          sampbuf_sync.fifo.push_back( (int8)(round_i( imag( capbuf[t] )*128) ) );
        }
        sampbuf_sync.fifo_peak_size=MAX(sampbuf_sync.fifo.size(),sampbuf_sync.fifo_peak_size);
        sampbuf_sync.condition.notify_one();
      }
      #endif
    } else {
      cout << "No valid device present.\n";
      ABORT(-1);
    }
  }

  // Successful exit. (Should never get here!)
  return 0;
}

