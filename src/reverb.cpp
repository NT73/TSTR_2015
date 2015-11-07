/******************************************/
/*
  reverb.cpp
  by Qiao Yu & Tatoud Nicolas.

  Implementation temp. & freq. reverb effect
  by convolution
*/
/******************************************/

#include "RtAudio.h"
#include "somefunc.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define DUPLEX 0
#define REVERB_TEMP 1
#define REVERB_FREQ 2

/*
typedef char MY_TYPE;
#define FORMAT RTAUDIO_SINT8

typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16

typedef S24 MY_TYPE;
#define FORMAT RTAUDIO_SINT24

typedef signed long MY_TYPE;
#define FORMAT RTAUDIO_SINT32

typedef float MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32
*/

typedef double MY_TYPE;
#define FORMAT RTAUDIO_FLOAT64

typedef struct {
    unsigned int bufferBytes;
    MY_TYPE* bufferImpres;
    unsigned int impresFrames;
    unsigned int bufferFrames;
    unsigned int convSize;
    MY_TYPE *bufferConv;
    //MY_TYPE *bufferLastConv;
  } userData;


void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage: reverb N fs <iDevice> <oDevice> <iChannelOffset> <oChannelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    iDevice = optional input device to use (default = 0),\n";
  std::cout << "    oDevice = optional output device to use (default = 0),\n";
  std::cout << "    iChannelOffset = an optional input channel offset (default = 0),\n";
  std::cout << "    and oChannelOffset = optional output channel offset (default = 0).\n\n";
  exit( 0 );
}


/***
* Routine de CallBack Audio
* Implementation de l'effet reverb à convolution en temps reel
***/

int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *data )
{
  // Since the number of input and output channels is equal, we can do
  // a simple buffer copy operation here.
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;
  
  unsigned int effet_audio = DUPLEX ;
  userData* pData = (userData *) data;
  unsigned int *bytes = (unsigned int *) &(pData->bufferBytes);
  unsigned int *impresFrames = (unsigned int *) &(pData->impresFrames);
  unsigned int *bufferFrames = (unsigned int *) &(pData->bufferFrames);
  unsigned int *convSize = (unsigned int *) &(pData->convSize);
  MY_TYPE* impres = (MY_TYPE *) (pData->bufferImpres);
  MY_TYPE* conv = (MY_TYPE *) (pData->bufferConv);
  //MY_TYPE* lastConv = (MY_TYPE *) (pData->bufferLastConv);
  
  // Mesure du temps initial
  double tStart, tEnd, deltaT;
  tStart = get_process_time();
  
  switch(effet_audio) {
  
  case DUPLEX :
  
    // Aucun traitement effectue
    memcpy( outputBuffer, inputBuffer, *bytes );
    
  break;
  
  case REVERB_TEMP :
  
    // Convolution temporelle
    MY_TYPE *tmpInputBuffer = (MY_TYPE *)inputBuffer;
    
    for(unsigned int i=0; i<*convSize;i++) {
      unsigned int kmin = (i>=*impresFrames)?i-*impresFrames+1:0;
      unsigned int kmax = (i<*bufferFrames)?i:*bufferFrames-1;
      MY_TYPE tmp = 0;
      
      for(unsigned int j=kmin;j<=kmax;j++)
      {
        tmp=tmp+tmpInputBuffer[j]*impres[i-j];
      }
      //conv[i] = tmp;
      //if(i<*impresFrames-1) {conv[i]+=lastConv[*bufferFrames+i];}
      conv[i]=(i<(*impresFrames-1))?tmp+conv[*bufferFrames+i]:tmp;
    }
    
    //memcpy( (void *)lastConv, (void *)conv, (*convSize)*sizeof(MY_TYPE) );
    memcpy( outputBuffer, (void *)conv, *bytes );
    
  break;
  
  /*
  case REVERB_FREQ :
    memcpy( outputBuffer, inputBuffer, *bytes );
  break ;
  */
  }
  
  tEnd = get_process_time();
  deltaT= tEnd - tStart ;
  std::cout << "t = " << deltaT << " s" << std::endl ;
  return 0 ;
}




int main( int argc, char *argv[] )
{
  unsigned int channels, fs, oDevice = 0, iDevice = 0, iOffset = 0, oOffset = 0;
  userData optionData;
  
  /***
  * Ouverture et lecture du fichier binaire (sans entete) contenant
  * la reponse impulsionnelle utilisee pour l'effet de reverb
  ***/
  
  // Ouverture du fichier de la reponse impulsionnelle
  FILE *ptr_impres = NULL ;
  unsigned int file_size ;
  
  ptr_impres = fopen ( "src/impres" , "rb" );
  if (ptr_impres==NULL) {fputs ("File error\n",stderr); exit (1);}
  
  // Determination de la taille du fichier
  fseek (ptr_impres , 0 , SEEK_END);
  file_size = ftell (ptr_impres);
  // Diminution du nombre d'echantillon de la reponse impulsionnelle
  file_size -= 70000*sizeof(MY_TYPE);
  rewind (ptr_impres);
  //std::cout << file_size << std::endl ;
  
  // Allocation de la mémoire de la reponse impulsionnelle
  optionData.bufferImpres = (MY_TYPE*) malloc (sizeof(char)*file_size);
  if (optionData.bufferImpres == NULL) {fputs ("Memory error",stderr); exit (2);}
  
  // Lecture du fichier
  optionData.impresFrames = file_size/sizeof(MY_TYPE);
  std::cout << "Taille reponse impulsonnelle : " << optionData.impresFrames << std::endl ;
  if (fread (optionData.bufferImpres,sizeof(MY_TYPE),optionData.impresFrames,ptr_impres) != optionData.impresFrames) { 
    fputs ("Reading error",stderr); 
    exit (3);
  }
  fclose(ptr_impres);
  
  /*
  // Verification de la reponse impulsionnelle
  for(unsigned int m=100;m<140;m++) {
    std::cout << m << " : " << optionData.bufferImpres[m] << std::endl;
  }
  */
  
  /***
  * Implementation des classes et methodes fournies par l'API RtAudio
  * pour le Traitement de Signal en Temps Reel
  ***/
  
  // Minimal command-line checking
  if (argc < 3 || argc > 7 ) usage();

  RtAudio adac;
  if ( adac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  channels = (unsigned int) atoi(argv[1]);
  fs = (unsigned int) atoi(argv[2]);
  if ( argc > 3 )
    iDevice = (unsigned int) atoi(argv[3]);
  if ( argc > 4 )
    oDevice = (unsigned int) atoi(argv[4]);
  if ( argc > 5 )
    iOffset = (unsigned int) atoi(argv[5]);
  if ( argc > 6 )
    oOffset = (unsigned int) atoi(argv[6]);

  // Let RtAudio print messages to stderr.
  adac.showWarnings( true );

  // Set the same number of channels for both input and output.
  unsigned int bufferFrames = 512;
  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = iDevice;
  iParams.nChannels = channels;
  iParams.firstChannel = iOffset;
  oParams.deviceId = oDevice;
  oParams.nChannels = channels;
  oParams.firstChannel = oOffset;

  if ( iDevice == 0 )
    iParams.deviceId = adac.getDefaultInputDevice();
  if ( oDevice == 0 )
    oParams.deviceId = adac.getDefaultOutputDevice();

  RtAudio::StreamOptions options;
  //options.flags |= RTAUDIO_NONINTERLEAVED;
  
  // Definition des membres de la structure fournie à la routine de CallBack Audio
  optionData.bufferFrames = bufferFrames;
  optionData.bufferBytes = bufferFrames * channels * sizeof( MY_TYPE );
  optionData.convSize = bufferFrames + optionData.impresFrames - 1;
  optionData.bufferConv = (MY_TYPE*) calloc (optionData.convSize,sizeof(MY_TYPE));
  if (optionData.bufferConv == NULL) {fputs ("Memory error",stderr); exit (2);}
  
  //optionData.bufferLastConv = (MY_TYPE*) calloc (optionData.convSize,sizeof(MY_TYPE));
  //if (optionData.bufferLastConv == NULL) {fputs ("Memory error",stderr); exit (2);}
  
  try {
    adac.openStream( &oParams, &iParams, FORMAT, fs, &bufferFrames, &inout, (void *)&optionData, &options );
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    exit( 1 );
  }

  // Test RtAudio functionality for reporting latency.
  std::cout << "\nStream latency = " << adac.getStreamLatency() << " frames" << std::endl;

  try {
    adac.startStream();

    char input;
    std::cout << "\nRunning ... press <enter> to quit (buffer frames = " << bufferFrames << ").\n";
    std::cin.get(input);

    // Stop the stream.
    adac.stopStream();
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

 cleanup:
  if ( adac.isStreamOpen() ) adac.closeStream();
  
  free(optionData.bufferImpres);
  free(optionData.bufferConv);
  //free(optionData.bufferLastConv);

  return 0;
}
