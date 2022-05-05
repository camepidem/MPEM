//InterVideo.h

//Contains the data for the Output Data Intervention

#ifndef INTERVIDEO_H
#define INTERVIDEO_H 1

#include "Intervention.h"

//Unless otherwise disabled:
#ifndef WANT_VIDEO
#define WANT_VIDEO 1
#endif

//Establish platform can support video output:
#if WANT_VIDEO
//WIN64 can't currently use video as allegro binaries are not compiled there, the WANT_VIDEO is turned off in the project settings: Properties->C/C++->Preprocessor->Preprocessor definitions
#define USE_VIDEO 1
#endif

#if USE_VIDEO

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include "ColourRamp.h"
#include "Raster.h"
#include "cfgutils.h"

#ifdef NDEBUG
#define AL_DLL_NAME "allegro-5.0.5-monolith-md.dll"
#else
#define AL_DLL_NAME "allegro-5.0.5-monolith-md-debug.dll"
#endif


typedef void (*SignalhandlerPointer)(int);

#endif

class InterVideo : public Intervention {

public:

	InterVideo();

	~InterVideo();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

#if USE_VIDEO

	int bVideoForceEnable;

	int bShowHostLandscape;
	int bVideoExportImages;
	int bVideoExportNoHostAsAlpha;

	ALLEGRO_BITMAP *outputBufferBmp;

	int bVideoExportLastframe;

	int bShowOnlySymptomatic;

	ALLEGRO_DISPLAY *displayInfection;
	ALLEGRO_DISPLAY *displayHost;

	char colourRampSourceFileDefaultString[_N_MAX_STATIC_BUFF_LEN];

	ColourRamp *colourRampHost;
	char colourRampHostSourceFile[_N_MAX_STATIC_BUFF_LEN];
	ColourRamp *colourRampHealthy;
	char colourRampHealthySourceFile[_N_MAX_STATIC_BUFF_LEN];
	ColourRamp *colourRampInfectious;
	char colourRampInfectiousSourceFile[_N_MAX_STATIC_BUFF_LEN];

	int screen_w;
	int screen_h;

	double viewX;
	double viewY;

	double zoomLevel;

	ALLEGRO_BITMAP *landMapBmp;
	ALLEGRO_FONT *defaultFont;

	void drawInfectionStateToRaster(ALLEGRO_BITMAP *drawTarget, bool bSetBlackAsAlpha=false, const Raster<int> *mask=NULL);

	//Time in ms since last frame was rendered:
	int timeLastFrame;

	//Framerate limiting and timing:
	double desiredLengthInSeconds;
	double simulationTimePerSecond;
	double FPSLimit;

	int bDisplayTime;
	int bDisplayWindSock;
	
	//Optionally mask out regions of the landscape:
	int bUseMask;
	std::map<double, Raster<int>> masks;

	void safeShutdownAllegro();

	//Raster visualisation methods:
	int writeRasterToFile(const char *title, Raster<int> *data, ColourRamp *ramp);

#endif

protected:


};

#endif
