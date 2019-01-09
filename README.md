An OpenCL accelerated TDD/FDD LTE Scanner (from rtlsdr/hackRF/bladeRF A/D samples to PDSCH output and RRC SIB messages decoded). By Jiao Xianjun (putaoshu@gmail.com). Tech blog: http://sdr-x.github.io

------------------------------
New features, make and Usages
------------------------------

**0x01. basic method to build program**

    mkdir build
    cd build
    cmake ../                   -- default for rtlsdr and OpenCL ON;   OR
    cmake ../ -DUSE_BLADERF=1   -- build for bladeRF;    OR
    cmake ../ -DUSE_HACKRF=1    -- build for hackRF
    cmake ../ -DUSE_OPENCL=0    -- disable OpenCL (See notes in later chapter)
    make

CellSearch and LTE-Tracker program will be generated in build/src. Use "--help" when invoke program to see all options.

(You may need some related libraries, such as itpp, fftw, libboost-, Curses, ... etc.) For most Ubuntu users, the following should be enough to get the necessary libraries installed:

    sudo apt install cmake libitpp-dev librtlsdr-dev libopenblas-dev libncurses5-dev

**0x02. basic usage (If you have OpenCL, make sure those .cl files in LTE-Cell-Scanner/src have been copy to program directory)**
            
*0x02.1* CellSearch --freq-start 1890000000   (try to search LTE Cell at 1890MHz)

    output:
    ...
    Detected a TDD cell! At freqeuncy 1890MHz, try 0
    cell ID: 253
    PSS ID: 1
    RX power level: -17.0064 dB
    residual frequency offset: -48.0366 Hz
                k_factor: 1
    ...
    Detected the following cells:
    Meaning -- DPX:TDD/FDD; A: #antenna ports C: CP type ; P: PHICH duration ; PR: PHICH resource type
    DPX  CID  A     fc  freq-offset RXPWR  C   nRB  P   PR  CrystalCorrectionFactor
    TDD  253  2  1890M         -48h   -17  N  100   N  1/2   0.99999997458380551763
    
*0x02.2* LTE-Tracker -f 1890000000  (try to track LTE Cell at 1890MHz)

*0x02.3* LTE_DL_receiver (Matlab script. Decode RRC SIB message in PDSCH by reading captured signal bin file)

*0x02.4* LTE_DL_receiver 1890 40 40 (Matlab script. Decode SIB at 1890MHz lively with LNA VGA gain of hackRF 40dB 40dB)

    output:
    ...
    TDD SFN-864 ULDL-2-|D|S|U|D|D|D|S|U|D|D| CID-216 nPort-2 CP-normal PHICH-DUR-normal-RES-1
    SF0 PHICH1 PDCCH1 RNTI: 
    ...
    SF4 PHICH1 PDCCH1 RNTI: SI-RNTI SI-RNTI 
    PDCCH   No.0  4CCE: Localized VRB from RB0 to RB11 MCS-7 HARQ-0 NEWind-0 RV-0 TPC-1 DAI-0
    Calling asn1c decoder (../asn1_test/LTE-BCCH-DL-SCH-decode/progname) for BCCH-DL-SCH-Message.
    ../asn1_test/LTE-BCCH-DL-SCH-decode/progname tmp_sib_info.per -p BCCH-DL-SCH-Message
    <BCCH-DL-SCH-Message>
        <message>
            <c1>
                <systemInformation>
                    <criticalExtensions>
                        <systemInformation-r8>
                            <sib-TypeAndInfo>
                                    <sib2>
                                        <radioResourceConfigCommon>
                                            <rach-ConfigCommon>
                                                <preambleInfo>
                                                    <numberOfRA-Preambles><n52/></numberOfRA-Preambles>
    ...

**0x03. Change gian by hand.**

Use "-g X" to set gain value X to hardware. If "-g" is not used, default values are used:

    rtlsdr default  0 (AGC)
    hackRF default  40dB (VGA gain, can be adjusted by "-g"), LAN gain is fixed at 40dB
    bladeRF default 60dB + maximum LNA gain. "-g" can set total gain which will be distributed to LNA VGA1 VGA2 automatically

gain is important to get good rx performance. If CellSearch reports like "input level: avg abs(real) 0.00594959 avg abs(imag) 0.00614887", where the numbers are far below 1, larger gain value should be considered.

**0x04. "-t" forces into original carrier-sampling-clock twisted mode.**

Without "-t" leads to non-twisted mode (default mode)

**0x05. Capture to and reload from raw bin file**

    "CellSearch --freq-start 1860e6 --recbin a.bin" saves signal in a.bin while doing cell search at frequency 1.86GHz
    "CellSearch --loadbin test/f1890_s1.92_g0_0.16s.bin" searches LTE cell in test/f1890_s1.92_g0_0.16s.bin

**0x06. "--num-try x" performs x tries of searching at each frequency.**

When signal is weak, only one try may not have you good luck.

**ATTENTION!!! Please use release version instead of dev trunk if you want a 100% workable program.**

----------------------------------------------------------------------
Notes
----------------------------------------------------------------------

**0x01. About carrier-sampling-clock twisted and non-twisted mode.**

Default mode supports external LNB and fast pre-search.
See detailed explanation in https://github.com/JiaoXianjun/rtl-sdr-LTE , and videos: (inside China): http://v.youku.com/v_show/id_XNjc1MjIzMDEy.html ,
 (outside China): http://www.youtube.com/watch?v=4zRLgxzn4Pc )

**0x02. About OpenCL.**

**0x02.1** Make sure OpenCL SDK (Intel, AMD, Nvidia) has been installed in your system correctly, if you want LTE Scanner accelerated by OpenCL.

**0x02.2 IMPORTANT!** Before run, those kernel files (src/*.cl) should be put IN THE SAME DIRECTORY AS executable program or in $PATH.
Because they need to be compiled and executed by OpenCL runtime after program launch.

**0x02.3 Test an OpenCL example:**

    CellSearch --loadbin test/f1890_s1.92_g0_0.16s.bin --opencl-platform=0 --opencl-device=0 --filter-workitem=32 --xcorr-workitem=2

OpenCL platform and device are specified by "--opencl-platform=idx1 --opencl-device=idx2" to decide which CPU/GPU is used.
When program runs, it tells you how many platforms are detected in total. Default index is 0.

Number of workitems for 6RB channel filter kernel can be specified by "--filter-workitem". Default value is 32.

Number of workitems in the 1st NDrange dimension for PSS frequency-time correlation kernel can be specified by "--xcorr-workitem".
(ATTENTION!!! Now it is omitted because #define FILTER_MCHN_SIMPLE_KERNEL in searcher.h. Turn the pre-define off to use "--xcorr-workitem")
Default value is 2. Number of workitems of PSS correlation'2nd-NDrange-dimension depends on PPM range (use "-p" to change it).

Optimal number of workitems is platform-device dependent. Optimal values should be found according to your computer configuration.

**0x02.4 ATTENTION!!!** If you got -4(CL_MEM_OBJECT_ALLOCATION_FAILURE) or -5(CL_OUT_OF_RESOURCES) error in some OpenCL platform-device, try smaller PPM value to narrow frequency offset range. Because less range less OpenCL work-items needed.

--------------------
News:
--------------------

2014-10: support bladeRF now.

2014-07: Now Matlab can parse SIB automatically by calling asn1c. See doc in asn1_test and http://sdr-x.github.io/LTE-SIB-decoding-by-asn1c/ . (Don't forget compling the progname in your own computer!)

2014-06: SIB message (output by LTE_DL_receiver.m) is decoded successfully! See Matlab/*SIB.txt.

2014-05: 20MHz LTE PDCCH DCI format1A for SI-RNTI has been detected successfully from HACKRF captured signal, and corresponding SIB PDSCH constellation is shown. Run: Matlab/LTE_DL_receiver.m
 (http://youtu.be/2JH_EGdHyYE  http://v.youku.com/v_show/id_XNzE3NDYwMDgw.html)

-----------------------------
Bakcups
------------------------------

Before explore this LTE Scanner, it's better for you to do some homeworks: (my blog: http://sdr-x.github.io/ if you have time)

1. Original FDD only LTE Cell Scanner / Tracker: https://github.com/Evrytania/LTE-Cell-Scanner , by James Peroulas.

2. rtl-sdr ultra cheap USB TV dongle: http://sdr.osmocom.org/trac/wiki/rtl-sdr

3. hackrf board: http://greatscottgadgets.com/hackrf/ , https://github.com/mossmann/hackrf

4. bladeRF board: http://www.nuand.com/  ,  https://github.com/Nuand/bladeRF

COMPATIBLE rtl-sdr and hackrf version (maybe not valid now. If you have issues, try these rev.):

librtlsdr (https://github.com/steve-m/librtlsdr) release v0.5.2 (Not v0.5.3 at least for my computer)

libhackrf (https://github.com/mossmann/hackrf  ) revision around 2014 April 1st.

See TODO if you want to contribute. Any questions or interests, feel free to contact me. putaoshu@gmail.com

Introduction video: (inside China) http://v.youku.com/v_show/id_XNjk3Mjc1MTUy.html ,
(outside China) http://www.youtube.com/watch?v=3hnlrYtjI-4

Introduction video before: (inside China) http://pan.baidu.com/s/1o6qbLGY ,
(outside China) http://www.youtube.com/watch?v=SxZzEVEKuRs

Introduction video before: (inside China) http://v.youku.com/v_show/id_XNjc1MjIzMDEy.html ,
(outside China) http://www.youtube.com/watch?v=4zRLgxzn4Pc

--------------------------------------------------------------------------
Original Readme of James Peroulas's LTE Cell Scanner / Tracker
--------------------------------------------------------------------------
LTE Cell Scanner / Tracker
--------------------------

This is a collection of tools to locate and track LTE basestation cells using
very low performance RF front ends. For example, these tools work with RTL2832
based dongles (E4000, R820T, etc.) which have a noise figure of 20dB, only 8
bits in the A/D, and a crystal with a frequency error of about 100 ppm.

The 'CellSearch' program can be used to search for LTE carriers within a range
of frequencies.  Once an LTE frequency has been located, 'LTE-Tracker' can be
used to monitor and track, in realtime, any LTE cells on that frequency.

The main documentation in html format can be found on the web at:
  http://www.evrytania.com/lte-tools
And in the doc/index.html subdirectory of this distribution.

For questions, comments, or bug reports, contact James Peroulas
james@evrytania.com, or check the bugtracker on github:
  https://github.com/Evrytania/LTE-Cell-Scanner/issues

------
Quick build instructions:
------
  cd LTE-Cell-Scanner
  mkdir build
  cd build
  cmake ..
  make

------
Install:
------
  sudo make install

------
Quick usage instructions:
------

Search for LTE carriers within a common LTE frequency range used in the US:
  CellSearch --freq-start 715e6 --freq-end 768e6

Start tracking LTE cells on frequency 739 MHz:
  LTE-Tracker -f 739000000

