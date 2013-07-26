#include <stdio.h>
#include <iostream>
#include <vector>
#include <iomanip> // setprecision
#include <chrono> // timing
#include <sys/resource.h>

#include <zmq.hpp>

//#include <mgl2/fltk.h>

#include "fft.h"
#include "eeg_receiver.h"
#include "gnuplot.h"

using namespace std;
using namespace chrono;

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

int main()
{
  // change the proiority and scheduler of process
  struct sched_param param;
  param.sched_priority = 99;
  if (sched_setscheduler(0, SCHED_FIFO, & param) != 0) {
    perror("sched_setscheduler");
    exit(EXIT_FAILURE);
  }

  // data to be received
  double time;
  size_t numChannels = 65;
  size_t numChannelsWOT = numChannels - 1;
  size_t numScans = 32;
  size_t dataSize = numChannels * numScans;
  float channels[dataSize];
  EegReceiver eeg(numChannels, numScans);

  // save received data into file
  FILE* pFile;
  pFile = fopen("data", "wb");
  setvbuf (pFile, NULL, _IOFBF, dataSize*sizeof(float));

  /*mglFLTK gr("test");
  mgl_fltk_thr();

  mglData data;
  data->Create(n,3);

  gr.SetOrigin(NAN,NAN,NAN);
  */

  size_t sampling_frq = 4800; // Hz
  size_t dft_points = 1024; // points

cout<<"l0"<<endl;
  Fft<double> fft(dft_points, Fft<double>::windowFunc::NONE, sampling_frq, numChannelsWOT);
cout<<"l1"<<endl;
  vector<vector<double> > powers(numChannels);
  vector<vector<double> > phases(numChannels);
  for (size_t i=0; i<numChannels; i++) {
    powers[i].resize(dft_points/2+1);
    phases[i].resize(dft_points/2+1);
  }
  vector<double> point(numChannelsWOT);

  struct timespec requestStart, requestEnd;


  double alpha = 0.9;

  const size_t numChannelsPlt = 4;
  vector<float> pwrPlt(numChannelsPlt);
  vector<string> legends;
  legends.push_back(string("ch 1"));
  legends.push_back(string("ch 2"));
  legends.push_back(string("ch 3"));
  legends.push_back(string("ch 4"));

  GnuPlot gnuplot(legends);

  // main loop
  int loop = 0;
  for(; !kbhit();){
    eeg.receive(time, channels);
    cout<<fixed<<setprecision(9)<<time<<endl;

    //cout<<"write in file"<<endl;

    /*clock_gettime(CLOCK_REALTIME, &requestEnd);
    double accum = ( requestEnd.tv_sec - requestStart.tv_sec )
      + ( requestEnd.tv_nsec - requestStart.tv_nsec )
      / 1E9;
    cout<<fixed<<setprecision(12)<<accum<<endl;*/

    for (size_t i=0; i<numScans; i++) {
      for (size_t ch=0; ch<numChannelsWOT; ch++) {
        point[ch] = channels[i*numChannels+ch];
      }
      // large laplacians
      point[62] = channels[27] - 0.25 * (channels[25]+channels[9]+channels[29]+channels[45]);
      point[63] = channels[31] - 0.25 * (channels[29]+channels[13]+channels[33]+channels[49]);
      fft.AddPoints(point);
    }

    loop += 1;
    if (loop > 1) {
      clock_gettime(CLOCK_REALTIME, &requestStart);
      if (fft.Process()) {
        fft.GetPower(powers);
        pwrPlt[0] = (alpha) * pwrPlt[0] + (1.0 - alpha) * (powers[27][3] - 0.25 * (powers[25][3]+powers[9][3]+powers[29][3]+powers[45][3]));
        pwrPlt[1] = (alpha) * pwrPlt[1] + (1.0 - alpha) * (powers[31][3] - 0.25 * (powers[29][3]+powers[13][3]+powers[33][3]+powers[49][3]));
        pwrPlt[2] = (alpha) * pwrPlt[2] + (1.0 - alpha) * powers[62][3];
        pwrPlt[3] = (alpha) * pwrPlt[3] + (1.0 - alpha) * powers[63][3];

        fwrite(channels, sizeof(float), dataSize, pFile);
        //fwrite(channels, sizeof(float), dataSize, pFilePwr);
      }
      clock_gettime(CLOCK_REALTIME, &requestEnd);
      double accum = ( requestEnd.tv_sec - requestStart.tv_sec )
        + ( requestEnd.tv_nsec - requestStart.tv_nsec )
        / 1E9;
      cout<<"t: "<<fixed<<setprecision(12)<<accum<<endl;
      loop = 0;
      gnuplot.Plot(pwrPlt);
    }
  }

  fclose(pFile);

  return 0;
}

