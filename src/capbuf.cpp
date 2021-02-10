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

#include <itpp/itbase.h>
#include <iomanip>
#include <sstream>
#include <queue>
#include <curses.h>
#include <boost/math/special_functions/gamma.hpp>
#include "common.h"
#include "capbuf.h"
#include "macros.h"
#include "itpp_ext.h"
#include "dsp.h"

#ifdef HAVE_RTLSDR
#include "rtl-sdr.h"
#endif // HAVE_RTLSDR

#ifdef HAVE_HACKRF
#include "hackrf.h"
#endif

#ifdef HAVE_BLADERF
#include <libbladeRF.h>
#endif

using namespace itpp;
using namespace std;

#ifdef HAVE_HACKRF

int8 hackrf_rx_buf[CAPLENGTH*2];  // used for capture_data() and hackrf rx callback
int hackrf_rx_count;  // used for capture_data() and hackrf rx callback

static int capbuf_hackrf_callback(hackrf_transfer* transfer) {
  size_t bytes_to_write;
  size_t hackrf_rx_count_new = hackrf_rx_count + transfer->valid_length;

  int count_left = (CAPLENGTH*2) - hackrf_rx_count_new;
  if ( count_left <= 0 ) {
    bytes_to_write = transfer->valid_length + count_left;
  } else {
    bytes_to_write = transfer->valid_length;
  }

//  cout << transfer->valid_length  << " " << hackrf_rx_count << " " << bytes_to_write << "\n";
  if (bytes_to_write!=0)
  {
    memcpy( hackrf_rx_buf+hackrf_rx_count, transfer->buffer, bytes_to_write );
//    for (size_t i=0; i<bytes_to_write; i++) {
//      hackrf_rx_buf[hackrf_rx_count+i] = transfer->buffer[i];
//    }
    hackrf_rx_count = hackrf_rx_count + bytes_to_write;
  }
//  cout << transfer->valid_length  << " " << hackrf_rx_count << " " << bytes_to_write << "\n";

  return(0);
}

#endif

#ifdef HAVE_BLADERF
int16 bladerf_rx_buf[CAPLENGTH*2];  // used for capture_data()
volatile bool do_exit = false;
int open_bladerf_board(bladerf_device * & bladerf_dev, unsigned int freq_hz, unsigned int buffer_size) {
  int status;
//  cout << "bladerf_dev " << bladerf_dev << "\n";
  status = bladerf_set_frequency(bladerf_dev, BLADERF_MODULE_RX, freq_hz);
  if (status != 0) {
    printf("open_bladerf_board bladerf_set_frequency: Failed to set frequency: %s\n",
            bladerf_strerror(status));
    return(-1);
  }

  status = bladerf_sync_config(bladerf_dev, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, 2, buffer_size, 1, 3500);
  if (status != 0) {
     printf("open_bladerf_board bladerf_sync_config: Failed to configure sync interface: %s\n",
             bladerf_strerror(status));
     return(-1);
  }

  status = bladerf_enable_module(bladerf_dev, BLADERF_MODULE_RX, true);
  if (status != 0) {
     printf("open_bladerf_board bladerf_enable_module: Failed to enable RX module: %s\n",
             bladerf_strerror(status));
     return(-1);
  }

  return(0);
}

int close_bladerf_board(bladerf_device * & bladerf_dev) {
  // Disable RX module, shutting down our underlying TX stream
  int status = bladerf_enable_module(bladerf_dev, BLADERF_MODULE_RX, false);
  if (status != 0) {
    printf("close_bladerf_board bladerf_enable_module: Failed to disable RX module: %s\n",
             bladerf_strerror(status));
    return(-1);
  }

  return(0);
}
#endif

#ifdef HAVE_RTLSDR
static void capbuf_rtlsdr_callback(
  unsigned char * buf,
  uint32_t len,
  void * ctx
) {
  //vector <char> & capbuf_raw = *((vector <char> *)ctx);
  //callback_package_t * cp=(callback_package_t *)ctx;
  callback_package_t * cp_p=(callback_package_t *)ctx;
  callback_package_t & cp=*cp_p;
  vector <unsigned char> * capbuf_raw_p=cp.buf;
  vector <unsigned char> & capbuf_raw=*capbuf_raw_p;
  rtlsdr_device * dev=cp.dev;

  if (len==0) {
    cerr << "Error: received no samples from USB device..." << endl;
    ABORT(-1);
  }

  for (uint32 t=0;t<len;t++) {
    //cout << capbuf_raw.size() << endl;
    if (capbuf_raw.size()<CAPLENGTH*2) {
      capbuf_raw.push_back(buf[t]);
    }
    if (capbuf_raw.size()==CAPLENGTH*2) {
      //cout << rtlsdr_cancel_async(dev) << endl;
      rtlsdr_cancel_async(dev);
      break;
    }
  }
  //cout << capbuf_raw.size() << endl;
}

// Declared in from_osmocom.cpp
double compute_fc_programmed(const double & fosc,const double & intended_flo);

double calculate_fc_programmed_in_context(
  // Inputs
  const double & fc_requested,
  const bool & use_recorded_data,
  const char * load_bin_filename,
  rtlsdr_device * & dev
) {
  double fc_programmed;
  bool load_bin_flag = (strlen(load_bin_filename)>4);
  if (use_recorded_data) {
    fc_programmed=fc_requested; // be careful about this! You should get fc_requested elsewhere
    cerr << "calculate_fc_programmed_in_context should not be called to calculate fc_programmed when using captured file.\n";
    ABORT(-1);
  }
  else if (load_bin_flag) {
    fc_programmed=fc_requested; // be careful about this! You should get fc_requested elsewhere
    cerr << "calculate_fc_programmed_in_context should not be called to calculate fc_programmed when using captured file.\n";
    ABORT(-1);
  } else {
    if (rtlsdr_get_tuner_type(dev)==RTLSDR_TUNER_E4000) {
      // This does not return the true center frequency, only the requested
      // center frequency.
      //fc_programmed=(double)rtlsdr_get_center_freq(dev);
      // Directly call some rtlsdr frequency calculation routines.
      fc_programmed=compute_fc_programmed(28.8e6,fc_requested);
      // For some reason, this will tame the slow time offset drift.
      // I don't know if this is a problem caused by the hardware or a problem
      // with the tracking algorithm.
      fc_programmed=fc_programmed+58;
      //MARK;
      //fc_programmed=fc_requested;
    } else {
      // Unsupported tuner...
      cout << "capture_data Warning: It is not RTLSDR_TUNER_E4000\n";
      cout << "set fc_programmed=fc_requested\n";
      fc_programmed=fc_requested;
    }
  }
  return(fc_programmed);
}
#endif // HAVE_RTLSDR

int write_header_to_bin(
  // input
  FILE * fp,
  const double & fc_requested,
  const double & fc_programmed,
  const double & fs_requested,
  const double & fs_programmed
) {

  int ret = 0;
  double valid_magic[8] = {73492.215, -0.7923597, -189978508, 93.126712, -53243.129, 0.0008123898, -6.0098321, 237.09983};
  uint64 tmp[8] = {(uint64)fc_requested, (uint64)fc_programmed, (uint64)fs_requested, (uint64)fs_programmed, 0,0,0,0};
  size_t num_write;
  for (uint16 i=0; i<8; i++) {
    num_write = fwrite(valid_magic+i, sizeof(double), 1, fp);
    num_write = num_write + fwrite(tmp+i, sizeof(uint64), 1, fp);

    if ( num_write != 2 ){
      ret = 1;
      cerr << "write_header_to_bin Error: write operation is not successful.\n";
      break;
    }
  }

  if (ret==0) {
    cout << "Write file header fc_requested " << fc_requested/1e6 << "MHz fc_programmed " << fc_programmed/1e6 << "MHz fs_requested " << fs_requested/1e6 << "MHz fs_programmed " << fs_programmed/1e6 << "MHz\n";
  }
  return(ret);
}

int read_header_from_bin(
  // input
  FILE * fp,
  // output, NAN represents invalid header info
  double & fc_requested,
  double & fc_programmed,
  double & fs_requested,
  double & fs_programmed
) {
  int ret;

  fc_requested = NAN;
  fc_programmed = NAN;
  fs_requested = NAN;
  fs_programmed = NAN;

//  FILE *fp = fopen(bin_filename, "rb");
//  if (fp == NULL)
//  {
//    cerr << "read_header_from_bin Error: unable to open file: " << bin_filename << endl;
//    ABORT(-1);
//  }

  double valid_magic[8] = {73492.215, -0.7923597, -189978508, 93.126712, -53243.129, 0.0008123898, -6.0098321, 237.09983};
  double magic[8];
  uint64 tmp[8];
  uint16 valid_count = 0;
  size_t num_read;
  for (uint16 i=0; i<8; i++) {
    num_read = fread(magic+i, sizeof(double), 1, fp);
    num_read = num_read + fread(tmp+i, sizeof(uint64), 1, fp);

    if ( num_read == 2 )
      valid_count = valid_count + ( magic[i]==valid_magic[i] );
  }

//  fclose(fp);

  if ( valid_count == 8 ) {
    fc_requested = tmp[0];
    fc_programmed = tmp[1];
    fs_requested = tmp[2];
    fs_programmed = tmp[3];
    ret = 0;
  } else {
    ret = 1;
  }

  return(ret);

}

int read_header_from_bin(
  // input
  const char *bin_filename,
  // output, NAN represents invalid header info
  double & fc_requested,
  double & fc_programmed,
  double & fs_requested,
  double & fs_programmed
) {

  int ret;

  fc_requested = NAN;
  fc_programmed = NAN;
  fs_requested = NAN;
  fs_programmed = NAN;

  FILE *fp = fopen(bin_filename, "rb");
  if (fp == NULL)
  {
    cerr << "read_header_from_bin Error: unable to open file: " << bin_filename << endl;
    ABORT(-1);
  }

  double valid_magic[8] = {73492.215, -0.7923597, -189978508, 93.126712, -53243.129, 0.0008123898, -6.0098321, 237.09983};
  double magic[8];
  uint64 tmp[8];
  uint16 valid_count = 0;
  size_t num_read;
  for (uint16 i=0; i<8; i++) {
    num_read = fread(magic+i, sizeof(double), 1, fp);
    num_read = num_read + fread(tmp+i, sizeof(uint64), 1, fp);

    if ( num_read == 2 )
      valid_count = valid_count + ( magic[i]==valid_magic[i] );
  }

  fclose(fp);

  if ( valid_count == 8 ) {
    fc_requested = tmp[0];
    fc_programmed = tmp[1];
    fs_requested = tmp[2];
    fs_programmed = tmp[3];
//    cout << "Read file header fc_requested " << fc_requested/1e6 << "MHz fc_programmed " << fc_programmed/1e6 << "MHz fs_requested " << fs_requested/1e6 << "MHz fs_programmed " << fs_programmed/1e6 << "MHz\n";
    ret = 0;
  } else {
    cout << "Read file header failed.\n";
    ret = 1;
  }

  return(ret);

}

// This function produces a vector of captured data. The data can either
// come from live data received by the RTLSDR, or from a file containing
// previously captured data.
// Also, optionally, this function can save each set of captured data
// to a file.
int capture_data(
  // Inputs
  const double & fc_requested,
  const double & correction,
  const bool & save_cap,
  const char * record_bin_filename,
  const bool & use_recorded_data,
  const char * load_bin_filename,
  const string & data_dir,
  rtlsdr_device * & rtlsdr_dev,
  hackrf_device * & hackrf_dev,
  bladerf_device * & bladerf_dev,
  const dev_type_t::dev_type_t & dev_use,
  // Output
  cvec & capbuf,
  double & fc_programmed,
  double & fs_programmed,
  const bool & read_all_in_bin // only for .bin file! if it is true, all data in bin file will be read in one time.
) {
  // Filename used for recording or loading captured data.
  static uint32 capture_number=0;
  stringstream filename;
  filename << data_dir << "/capbuf_" << setw(4) << setfill('0') << capture_number << ".it";

//  cout << use_recorded_data << "\n";
//  cout << load_bin_filename << "\n";

  bool record_bin_flag = (strlen(record_bin_filename)>4);
  bool load_bin_flag = (strlen(load_bin_filename)>4);

//  cout << load_bin_flag << "\n";

  int run_out_of_data = 0;

  if (use_recorded_data) {
    // Read data from a file. Do not use live data.
    if (verbosity>=2) {
      cout << "Reading captured data from file: " << filename.str() << endl;
    }

    it_ifile itf(filename.str());
    itf.seek("capbuf");
    itf>>capbuf;

    itf.seek("fc");
    ivec fc_v;
    itf>>fc_v;

    itf.seek("fcp");
    ivec fc_p;
    itf>>fc_p;

    itf.seek("fs");
    ivec fs_p;
    itf>>fs_p;

    if (fc_requested!=fc_v(0)) {
      cout << "capture_data Warning: while reading capture buffer " << capture_number << ", the read" << endl;
      cout << "center frequency did not match the expected center frequency." << endl;
    }
    itf.close();

//    fc_programmed=fc_requested; // be careful about this!
//    fc_programmed = calculate_fc_programmed_in_context(fc_requested, use_recorded_data, load_bin_filename, rtlsdr_dev);
    fc_programmed = fc_p(0);
    fs_programmed = fs_p(0);

  } else if (load_bin_flag) {
    // Read data from load_bin_filename. Do not use live data.
    // Convert to complex
    double fc_requested_tmp,fc_programmed_tmp,fs_requested_tmp,fs_programmed_tmp;
    bool header_exist = false;
    if ( read_header_from_bin(load_bin_filename, fc_requested_tmp,fc_programmed_tmp,fs_requested_tmp,fs_programmed_tmp) ) {
      cerr << "capture_data Error: read_header_from_bin failed.\n";
      ABORT(-1);
    }
    if (fc_requested_tmp!=NAN) {
      header_exist = true;
      if (fc_requested!=fc_requested_tmp) {
        cout << "capture_data Warning: while reading capture bin file " << load_bin_filename << ", the read" << endl;
        cout << "center frequency did not match the expected center frequency." << endl;
        cout << "fc_requested and fc_programmed in the file header will be omitted." << endl;
      }
    }

    unsigned char *capbuf_raw = new unsigned char[2*CAPLENGTH];

    if (read_all_in_bin) {
      capbuf.set_size(0, false);

      FILE *fp = fopen(load_bin_filename, "rb");
      if (fp == NULL)
      {
        cerr << "capture_data Error: unable to open file: " << load_bin_filename << endl;
        ABORT(-1);
      }

      if (header_exist) {// skip header
        if ( read_header_from_bin(fp, fc_requested_tmp,fc_programmed_tmp,fs_requested_tmp,fs_programmed_tmp) ) {
          cerr << "capture_data Error: skipping file header failed.\n";
          fclose(fp);
          ABORT(-1);
        }
      }

      uint32 len_capbuf = 0;
      while(true) { // read until run out of data
        int read_count = fread(capbuf_raw, sizeof(unsigned char), 2*CAPLENGTH, fp);

        len_capbuf = len_capbuf + CAPLENGTH;
        capbuf.set_size(len_capbuf, true);
        for (uint32 t=0;t<CAPLENGTH;t++) {
          uint32 i = len_capbuf-CAPLENGTH+t;
          capbuf(i)=complex<double>((((double)capbuf_raw[(t<<1)])-128.0)/128.0,(((double)capbuf_raw[(t<<1)+1])-128.0)/128.0);
        }
        if (read_count != (2*CAPLENGTH))
        {
//          cerr << "Run of recorded file data.\n";
          break;
//          ABORT(-1);
        }
      }

      fclose(fp);
    } else {
      capbuf.set_size(CAPLENGTH, false);

      FILE *fp = fopen(load_bin_filename, "rb");
      if (fp == NULL)
      {
        cerr << "capture_data Error: unable to open file: " << load_bin_filename << endl;
        ABORT(-1);
      }

      if (header_exist) {// skip header
        if ( read_header_from_bin(fp, fc_requested_tmp,fc_programmed_tmp,fs_requested_tmp,fs_programmed_tmp) ) {
          cerr << "capture_data Error: skipping file header failed.\n";
          fclose(fp);
          ABORT(-1);
        }
      }

      for(uint16 i=0; i<(capture_number+1); i++) {
        int read_count = fread(capbuf_raw, sizeof(unsigned char), 2*CAPLENGTH, fp);
        if (read_count != (2*CAPLENGTH))
        {
//          fclose(fp);
//          cerr << "Error: file " << load_bin_filename << " size is not sufficient" << endl;
          cerr << "capture_data: Run of recorded file data.\n";
          run_out_of_data = 1;
          break;
//          ABORT(-1);
        }
      }
      fclose(fp);
      for (uint32 t=0;t<CAPLENGTH;t++) {
        capbuf(t)=complex<double>((((double)capbuf_raw[(t<<1)])-128.0)/128.0,(((double)capbuf_raw[(t<<1)+1])-128.0)/128.0); // 127 --> 128.
      }
    }

    delete[] capbuf_raw;

//    fc_programmed=fc_requested; // be careful about this!
//    fc_programmed = calculate_fc_programmed_in_context(fc_requested, use_recorded_data, load_bin_filename, rtlsdr_dev);
    fc_programmed = fc_programmed_tmp;
    fs_programmed = fs_programmed_tmp;
  } else {
    if (verbosity>=2) {
      cout << "Capturing live data" << endl;
    }

    if (dev_use == dev_type_t::RTLSDR) {
      #ifdef HAVE_RTLSDR
      // Calculate the actual center frequency that was programmed.
      fc_programmed = calculate_fc_programmed_in_context(fc_requested, use_recorded_data, load_bin_filename, rtlsdr_dev);

      // Center frequency
      uint8 n_fail=0;
  //    while (rtlsdr_set_center_freq(rtlsdr_dev,itpp::round(fc_requested*correction))<0) {
      while (rtlsdr_set_center_freq(rtlsdr_dev,itpp::round(fc_programmed*correction))<0) {
        n_fail++;
        if (n_fail>=5) {
          cerr << "capture_data Error: unable to set center frequency" << endl;
          ABORT(-1);
        }
        cerr << "capture_data: Unable to set center frequency... retrying..." << endl;
        sleep(1);
      }

  //    // Calculate the actual center frequency that was programmed.
  //    if (rtlsdr_get_tuner_type(rtlsdr_dev)==RTLSDR_TUNER_E4000) {
  //      // This does not return the true center frequency, only the requested
  //      // center frequency.
  //      //fc_programmed=(double)rtlsdr_get_center_freq(rtlsdr_dev);
  //      // Directly call some rtlsdr frequency calculation routines.
  //      fc_programmed=compute_fc_programmed(28.8e6,fc_requested);
  //      // For some reason, this will tame the slow time offset drift.
  //      // I don't know if this is a problem caused by the hardware or a problem
  //      // with the tracking algorithm.
  //      fc_programmed=fc_programmed+58;
  //      //MARK;
  //      //fc_programmed=fc_requested;
  //    } else {
  //      // Unsupported tuner...
  //      cout << "capture_data Warning: It is not RTLSDR_TUNER_E4000\n";
  //      cout << "set fc_programmed=fc_requested\n";
  //      fc_programmed=fc_requested;
  //    }
  //
  //    // Reset the buffer
  //    if (rtlsdr_reset_buffer(rtlsdr_dev)<0) {
  //      cerr << "capture_data Error: unable to reset RTLSDR buffer" << endl;
  //      ABORT(-1);
  //    }

      // Read and store the data.
      // This will block until the call to rtlsdr_cancel_async().
      vector <unsigned char> capbuf_raw;
      capbuf_raw.reserve(CAPLENGTH*2);
      callback_package_t cp;
      cp.buf=&capbuf_raw;
      cp.dev=rtlsdr_dev;

      rtlsdr_read_async(rtlsdr_dev,capbuf_rtlsdr_callback,(void *)&cp,0,0);

      // Convert to complex
      capbuf.set_size(CAPLENGTH, false);
  #ifndef NDEBUG
      capbuf=NAN;
  #endif
      for (uint32 t=0;t<CAPLENGTH;t++) {
        // Normal
        capbuf(t)=complex<double>((((double)capbuf_raw[(t<<1)])-128.0)/128.0,(((double)capbuf_raw[(t<<1)+1])-128.0)/128.0);
        //// 127 --> 128.
        // Conjugate
        //capbuf(t)=complex<double>((capbuf_raw[(t<<1)]-127.0)/128.0,-(capbuf_raw[(t<<1)+1]-127.0)/128.0);
        // Swap I/Q
        //capbuf(t)=complex<double>((capbuf_raw[(t<<1)+1]-127.0)/128.0,(capbuf_raw[(t<<1)]-127.0)/128.0);
        // Swap I/Q and conjugate
        //capbuf(t)=complex<double>((capbuf_raw[(t<<1)+1]-127.0)/128.0,-(capbuf_raw[(t<<1)]-127.0)/128.0);
      }
      //cout << "capbuf power: " << db10(sigpower(capbuf)) << " dB" << endl;

      #endif // HAVE_RTLSDR
    } else if (dev_use == dev_type_t::HACKRF) {

      #ifdef HAVE_HACKRF

      fc_programmed = fc_requested;

      // Center frequency
      int result = hackrf_set_freq(hackrf_dev, fc_requested);
      if( result != HACKRF_SUCCESS ) {
        printf("hackrf_set_freq() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        ABORT(-1);
      }

      result = hackrf_stop_rx(hackrf_dev);
      if( result != HACKRF_SUCCESS ) {
        printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        ABORT(-1);
      }

      hackrf_rx_count = 0; // clear counter
      result = hackrf_start_rx(hackrf_dev, capbuf_hackrf_callback, NULL);

      if( result != HACKRF_SUCCESS ) {
        printf("hackrf_start_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
        ABORT(-1);
      }

      while(hackrf_is_streaming(hackrf_dev) == HACKRF_TRUE) {
//        cout << hackrf_rx_count << "\n";
        if( hackrf_rx_count == (CAPLENGTH*2) )
          break;
      }

      result = hackrf_is_streaming(hackrf_dev);

//      result = hackrf_stop_rx(hackrf_dev);
//      if( result != HACKRF_SUCCESS ) {
//        printf("hackrf_stop_rx() failed: %s (%d)\n", hackrf_error_name((hackrf_error)result), result);
//        ABORT(-1);
//      }

      // Convert to complex
      capbuf.set_size(CAPLENGTH, false);
      for (uint32 t=0;t<CAPLENGTH;t++) {
//        capbuf(t)=complex<double>((((double)hackrf_rx_buf[(t<<1)])-128.0)/128.0,(((double)hackrf_rx_buf[(t<<1)+1])-128.0)/128.0);
        capbuf(t)=complex<double>((((double)hackrf_rx_buf[(t<<1)])-0.0)/128.0,(((double)hackrf_rx_buf[(t<<1)+1])-0.0)/128.0);
      }

      #endif
    }  else if (dev_use == dev_type_t::BLADERF) {
      #ifdef HAVE_BLADERF
      int status = 0;
      fc_programmed = fc_requested;
      // open the board-----------------------------------------
      if (open_bladerf_board(bladerf_dev, fc_requested, CAPLENGTH) == -1) {
        printf("capture_data: open_bladerf_board() failed\n");
        ABORT(-1);
      }

      // Receive samples
      status = bladerf_sync_rx(bladerf_dev, (void *)bladerf_rx_buf, CAPLENGTH, NULL, 3500);
      if (status != 0) {
        printf("capture_data: bladerf_sync_rx : Failed to RX samples 1: %s\n",
                 bladerf_strerror(status));
        ABORT(-1);
      }

      if (do_exit)
      {
        printf("\ncapture_data: bladerf_sync_rx: Exiting...\n");
        ABORT(-1);
      }

      // close the board---------------------------------------
      if (close_bladerf_board(bladerf_dev) == -1) {
        printf("capture_data: close_bladerf_board() failed\n");
        ABORT(-1);
      }

      // Convert to complex
      capbuf.set_size(CAPLENGTH, false);
      for (uint32 t=0;t<CAPLENGTH;t++) {
        capbuf(t)=complex<double>((((double)bladerf_rx_buf[(t<<1)])-0.0)/2048.0,(((double)bladerf_rx_buf[(t<<1)+1])-0.0)/2048.0);
      }
      #endif
    }
  }

  // Save the capture data, if requested.
  if (save_cap) {
    if (verbosity>=2) {
      cout << "Saving captured data to file: " << filename.str() << endl;
    }
    it_file itf(filename.str(),true);
    itf << Name("capbuf") << capbuf;

    ivec fc_v(1);
    fc_v(0)=fc_requested;
    itf << Name("fc") << fc_v;

    ivec fc_p(1);
    fc_p(0)=fc_programmed;
    itf << Name("fcp") << fc_p;

    ivec fs_p(1);
    fs_p(0)=fs_programmed;
    itf << Name("fsp") << fs_p;

    itf.close();
  }

  if (record_bin_flag) {
    if (verbosity>=2) {
      cout << "Saving captured data to file: " << record_bin_filename << endl;
    }
    FILE *fp = NULL;
    if (capture_number==0){
      fp = fopen(record_bin_filename, "wb");
    } else {
      fp = fopen(record_bin_filename, "ab");
    }

    if (fp == NULL)
    {
      cerr << "capture_data Error: unable to open file: " << record_bin_filename << endl;
      ABORT(-1);
    }

    if (capture_number==0){ // write bin file header
      int ret = write_header_to_bin(fp, fc_requested,fc_programmed,(const double &)1920000,fs_programmed); // not use fs. it seems always 1920000
      if (ret) {
        cerr << "capture_data Error: unable write header info to file: " << record_bin_filename << endl;
        ABORT(-1);
      }
    }

    for (uint32 t=0;t<CAPLENGTH;t++) {
      unsigned char tmp;
      tmp = (unsigned char)( capbuf(t).real()*128.0 + 128.0 );
      fwrite(&tmp, sizeof(unsigned char), 1, fp);
      tmp = (unsigned char)( capbuf(t).imag()*128.0 + 128.0 );
      fwrite(&tmp, sizeof(unsigned char), 1, fp);
    }
    fclose(fp);
  }

  capture_number++;
  return(run_out_of_data);
}

