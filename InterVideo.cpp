//InterVideo.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "json/json.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "LocationMultiHost.h"
#include "DispersalManager.h"
#include "DispersalKernel.h"
#include "InterOutputDataDPC.h"
#include "InterVideo.h"


#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterVideo::InterVideo() {
	timeSpent = 0;
	InterName = "Video";

	//Set up Intervention:
	frequency = 1e30;
	timeFirst = world->timeEnd + 1e30;
	timeNext = timeFirst;
	enabled = 0;

#if USE_VIDEO

	setCurrentWorkingSubection(InterName);

	bVideoForceEnable = 0;

	//Memory housekeeping:
	colourRampHost = NULL;
	colourRampHealthy = NULL;
	colourRampInfectious = NULL;
	defaultFont = NULL;
	landMapBmp = NULL;
	outputBufferBmp = NULL;

	//Read Data:
	readValueToProperty("VideoEnable", &enabled, -1, "[0,1]");

	//TODO: Read all input values
	//TODO: Separate reading from action
	int bGaveDesiredLength;
	int bGaveSimTPerSec;

	char szMaskName[N_MAXSTRLEN];

	if (enabled) {

		readValueToProperty("VideoForceEnable", &bVideoForceEnable, -2, "[0,1]");
		if (bVideoForceEnable) {
			printk("Warning: Forcing video output on.\n");
		}

		//Default to output as frequently as the DPC data:
		frequency = 1.0;
		readValueToProperty("VideoFrequency", &frequency, 2, ">0");
		if (frequency <= 0.0) {
			reportReadError("ERROR: VideoFrequency must be > 0.0\n");
		}

		timeFirst = 1.0;
		readValueToProperty("VideotimeFirst", &timeFirst, 2, ">0");
		if (timeFirst < world->timeStart) {
			double periodsBehind = (world->timeStart - timeFirst) / frequency;
			if (periodsBehind - floor(periodsBehind) < 0.01 * frequency) {//Avoid floating point rounding shifting by a period when we should be exactly aligned to timeStart
				periodsBehind = floor(periodsBehind);
			}
			int nPeriodsBehind = ceil(periodsBehind);
			double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

			timeFirst = newTimeFirst;
		}


		//When to display:
		//Set up Intervention:
		timeNext = timeFirst;

		//Make the simulation play out in about 10s
		desiredLengthInSeconds = 10.0;
		simulationTimePerSecond = (world->timeEnd - world->timeStart) / desiredLengthInSeconds;

		bGaveDesiredLength = readValueToProperty("VideoDesiredLengthInSeconds", &desiredLengthInSeconds, -2, ">0");

		bGaveSimTPerSec = readValueToProperty("VideoSimulationTimePerSecond", &simulationTimePerSecond, -2, ">0");

		if (bGaveDesiredLength && bGaveSimTPerSec && !bIsKeyMode()) {
			reportReadError("ERROR: Please supply at most one of [VideoDesiredLengthInSeconds, VideoSimulationTimePerSecond]\n");
		}


		viewX = 0.0;
		viewY = 0.0;

		readValueToProperty("VideoX", &viewX, -2, "[0,#nCols#)");
		readValueToProperty("VideoY", &viewY, -2, "[0,#nRows#)");

		if (viewX < 0.0) viewX = 0.0;
		if (viewY < 0.0) viewY = 0.0;

		if (viewX > world->header->nCols - 1) viewX = world->header->nCols - 1;
		if (viewY > world->header->nRows - 1) viewY = world->header->nRows - 1;

		const int screen_W_default = 600;
		const int screen_H_default = 600;

		screen_w = screen_W_default;
		screen_h = screen_H_default;

		int nTinyRes = 100;
		int nMassiveRes = 4096;

		char screenResRange[1024];
		sprintf(screenResRange, "[%d, %d]", nTinyRes, nMassiveRes);

		readValueToProperty("VideoResolutionX", &screen_w, -2, screenResRange);
		readValueToProperty("VideoResolutionY", &screen_h, -2, screenResRange);

		//Check that this resolution wont immediately melt the graphics card:
		if (screen_w < nTinyRes) screen_w = nTinyRes;
		if (screen_h < nTinyRes) screen_h = nTinyRes;

		if (screen_w > nMassiveRes) screen_w = nMassiveRes;
		if (screen_h > nMassiveRes) screen_h = nMassiveRes;


		//Make display the same size as the size of the raster to begin with:
		//Fit such that initially whole map is on screen?
		double minRatio = min(double(screen_w) / double(world->header->nCols), double(screen_h) / double(world->header->nRows));
		//Don't allow zooming below 1 pixel per location:
		zoomLevel = max(minRatio, 1.0);
		readValueToProperty("VideoZoom", &zoomLevel, -2, ">=1");

		//Make sure zoom is reasonable:
		if (zoomLevel < 1) {
			zoomLevel = 1.0;
		}


		bVideoExportImages = 0;
		readValueToProperty("VideoExportImages", &bVideoExportImages, -2, "[0,1]");

		bVideoExportNoHostAsAlpha = 0;
		readValueToProperty("VideoExportNoHostAsAlpha", &bVideoExportNoHostAsAlpha, -3, "[0,1]");

		bVideoExportLastframe = 0;
		readValueToProperty("VideoExportLastFrame", &bVideoExportLastframe, -2, "[0,1]");

		bShowHostLandscape = 1;
		readValueToProperty("VideoShowHostLandscape", &bShowHostLandscape, -2, "[0,1]");

		bShowOnlySymptomatic = 0;
		readValueToProperty("VideoShowOnlySymptomatic", &bShowOnlySymptomatic, -2, "[0,1]");

		bDisplayTime = 1;
		readValueToProperty("VideoDisplayTime", &bDisplayTime, -2, "[0,1]");

		bDisplayWindSock = 0;
		readValueToProperty("VideoDisplayWindSock", &bDisplayWindSock, -2, "[0,1]");

		bUseMask = 0;
		sprintf(szMaskName, "%s%s%s", world->szPrefixLandscapeRaster, "VideoMask", world->szSuffixTextFile);
		if (readValueToProperty("VideoMask", szMaskName, -2, "#FileName#")) {
			bUseMask = 1;
		}

		sprintf(colourRampSourceFileDefaultString, "DEFAULT");

		sprintf(colourRampHostSourceFile, "%s", colourRampSourceFileDefaultString);
		readValueToProperty("VideoHostColourRampFileName", colourRampHostSourceFile, -2, "#FileName#");

		sprintf(colourRampHealthySourceFile, "%s", colourRampSourceFileDefaultString);
		readValueToProperty("VideoHealthyColourRampFileName", colourRampHealthySourceFile, -2, "#FileName#");

		sprintf(colourRampInfectiousSourceFile, "%s", colourRampSourceFileDefaultString);
		readValueToProperty("VideoInfectiousColourRampFileName", colourRampInfectiousSourceFile, -2, "#FileName#");

	}


	//Set up intervention:
	if (enabled && !world->bGiveKeys) {


		//Memory housekeeping:
		displayInfection = NULL;
		displayHost = NULL;
		landMapBmp = NULL;
		outputBufferBmp = NULL;

#ifdef _WIN32
		if (!checkPresent(AL_DLL_NAME)) {
			printk("WARNING: %s not found in local directory, if allegro is not installed somewhere on the system an unavoidable crash will happen right now.\nTo fix this, find this required file and place it in the working directory, or install allegro.\n", AL_DLL_NAME);
		}
#endif // _WIN32

		//Initialisation:
		if (!al_init()) {
			reportReadError("ERROR: Failed to initialize Graphics Library!\n");
		} else {

			if (!al_init_primitives_addon()) {
				reportReadError("ERROR: Failed to initialize Graphics Library primitives!\n");
			}


			if (!al_init_image_addon()) {
				reportReadError("ERROR: Failed to initialize Graphics Library Image export!\n");
			}

			al_init_font_addon();
			al_init_ttf_addon();

			defaultFont = al_load_ttf_font("Times.ttf", 24, 0);
			if (!defaultFont) {
				reportReadError("ERROR: Could not find default font\n");
			}

			//Intervention Intialisation:

			if (bGaveDesiredLength) {
				simulationTimePerSecond = (world->timeEnd - world->timeStart) / desiredLengthInSeconds;
			}

			if (bGaveSimTPerSec) {
				//Nothing extra to do...
			}

			//printk("SIM TIME PER SEC IS %f\n", simulationTimePerSecond);
			FPSLimit = simulationTimePerSecond / frequency;
			//printk("FPSLIMIT IS %f\n", FPSLimit);
			timeLastFrame = clock_ms();




			//TODO: (dis?)Allow non integer zoom level or find way to avoid aliasing and eyebleeding...

			//TODO: show host landscape map

			//TODO: force enable handling elsewhere


			//Masking:
			if (bUseMask) {

				bool bFoundFormat = false;

				//Try as JSON:
				Json::Reader reader;
				Json::Value root;

				if (reader.parse(szMaskName, root)) {
					//Looks like a json object:
					bFoundFormat = true;

					if (!root.isObject()) {
						reportReadError("ERROR: VideoMask unable to parse JSON to JSON object: %s\n", szMaskName);
						return;
					}

					for (auto itr = root.begin(); itr != root.end(); ++itr) {

						std::stringstream ssKey;
						ssKey << itr.key();
						std::string sKey = ssKey.str();
						std::replace(sKey.begin(), sKey.end(), '\"', ' ');

						std::stringstream ssClean;
						ssClean << sKey;

						double time;// = itr.key().asDouble();

						if (!(ssClean >> time)) {
							reportReadError("ERROR: Time: %s is not a valid simulation time\n", ssClean.str().c_str());
							return;
						}


						std::string targetDataFileName;
						auto jsonVal = *itr;

						if (jsonVal.isString()) {
							targetDataFileName = jsonVal.asString();
						} else {
							reportReadError("ERROR: Unable to read VideoMask raster file name: %s.\n", targetDataFileName.c_str());
							return;
						}

						Raster<int> mask;

						if (!mask.init(targetDataFileName)) {
							reportReadError("ERROR: Unable to read VideoMask raster file %s.\n", targetDataFileName.c_str());
							return;
						}

						if (!mask.header.sameAs(*world->header)) {
							reportReadError("ERROR: VideoMask raster file %s header does not match simulation.\n", targetDataFileName.c_str());
							return;
						}

						if (masks.find(time) != masks.end()) {
							reportReadError("ERROR: VideoMask cannot use the same time (%.6f) twice.\n", time);
							return;
						}

						for (const auto &myPair : masks) {

							double dDataTime = myPair.first;

							if (dDataTime - time == 0.0) {
								reportReadError("ERROR: Time: %.6f is present more than once in the set of VideoMask times. Repetitions of the exact same time are not allowed (even if string representations are distinct).\n", dDataTime);
								return;
							}

						}

						//Finally, actually able to use the data:
						masks[time] = mask;
					}

				}


				if (!bFoundFormat) {
					//Wasn't JSON, try as single filename:
					Raster<int> mask;

					if (mask.init(szMaskName)) {
						bFoundFormat = true;

						//Use this mask from time zero onwards:
						masks[0.0] = mask;
					}
				}
				
				if (!bFoundFormat) {
					reportReadError("ERROR: Video unable to load mask, either specify a single file name representing a raster, or a JSON object containing pairs of times and files.\nRead VideoMask %s\n", szMaskName);
				}

			}


			//Create colour ramps:
			if (strcmp(colourRampHostSourceFile, colourRampSourceFileDefaultString) == 0) {
				colourRampHost = new ColourRamp();
			} else {
				colourRampHost = new ColourRamp(colourRampHostSourceFile);
			}

			if (strcmp(colourRampHealthySourceFile, colourRampSourceFileDefaultString) == 0) {
				colourRampHealthy = new ColourRamp("H");
			} else {
				colourRampHealthy = new ColourRamp(colourRampHealthySourceFile);
			}

			if (strcmp(colourRampInfectiousSourceFile, colourRampSourceFileDefaultString) == 0) {
				colourRampInfectious = new ColourRamp("I");
			} else {
				colourRampInfectious = new ColourRamp(colourRampInfectiousSourceFile);
			}

			//Host Index:
			int initialX = screen_w*0.1;
			int initialY = screen_h*0.1;
			if (bShowHostLandscape) {
				al_set_new_window_position(initialX, initialY);
				initialX += 1.05*screen_w;

				displayHost = al_create_display(screen_w, screen_h);
				if (!displayHost) {
					reportReadError("ERROR: Video failed to create display for Host\n");
				}



				al_set_window_title(displayHost, "Host Index");

				al_clear_to_color(al_map_rgb(0, 0, 0));

				//Draw host index to screen:
				for (int rX = viewX; rX < min(int(viewX + ceil(screen_w / zoomLevel)), world->header->nCols); rX++) {
					for (int rY = viewY; rY < min(int(viewY + ceil(screen_h / zoomLevel)), world->header->nRows); rY++) {
						if (world->locationArray[rX + rY*world->header->nCols] != NULL) {
							double dense = world->locationArray[rX + rY*world->header->nCols]->getCoverage(-1);
							//al_draw_filled_rectangle((rX-viewX)*zoomLevel, (rY-viewY)*zoomLevel, (rX-viewX + 1)*zoomLevel, (rY-viewY + 1)*zoomLevel, al_map_rgb_f(dense, dense, dense));
							al_draw_filled_rectangle((rX - viewX)*zoomLevel, (rY - viewY)*zoomLevel, (rX - viewX + 1)*zoomLevel, (rY - viewY + 1)*zoomLevel, al_map_rgb(colourRampHost->getRed(dense), colourRampHost->getGreen(dense), colourRampHost->getBlue(dense)));
						}
					}
				}



				if (bVideoExportImages) {
					char pngName[_N_MAX_STATIC_BUFF_LEN];

					sprintf(pngName, "%s%s.%s", szPrefixOutput(), "TOTAL_COVERAGE", "png");
					al_save_bitmap(pngName, al_get_backbuffer(displayHost));
				}


				al_flip_display();

			}

			//Infection
			al_set_new_window_position(initialX, initialY);

			displayInfection = al_create_display(screen_w, screen_h);
			if (!displayInfection) {
				reportReadError("ERROR: Video failed to create display for Infection\n");
			}

			al_set_window_title(displayInfection, "Infection Status");

			al_clear_to_color(al_map_rgb(0, 0, 0));

			al_flip_display();

			//Assign the landBitmap:
			//landMapBmp = al_create_bitmap(world->header->nCols, world->header->nRows);
			landMapBmp = al_create_bitmap(screen_w, screen_h);
			if (!landMapBmp) {
				reportReadError("ERROR: Video failed to create map bitmap!\n");
			}

			if (bVideoExportNoHostAsAlpha) {
				outputBufferBmp = al_create_bitmap(screen_w, screen_h);
				if (!outputBufferBmp) {
					reportReadError("ERROR: Video failed to create output buffer bitmap!\n");
				}
			}

		}

	}

#endif

}

InterVideo::~InterVideo() {

#if USE_VIDEO
	safeShutdownAllegro();
#endif

}

#if USE_VIDEO
void InterVideo::safeShutdownAllegro() {

	if (enabled && !world->bGiveKeys) {
		al_shutdown_primitives_addon();

		if (displayInfection != NULL) {
			al_destroy_display(displayInfection);
			displayInfection = NULL;
		}

		if (displayHost != NULL) {
			al_destroy_display(displayHost);
			displayHost = NULL;
		}

		if (landMapBmp != NULL) {
			al_destroy_bitmap(landMapBmp);
			landMapBmp = NULL;
		}

		if (outputBufferBmp != NULL) {
			al_destroy_bitmap(outputBufferBmp);
			outputBufferBmp = NULL;
		}

		if (colourRampHost != NULL) {
			delete colourRampHost;
			colourRampHost = NULL;
		}

		if (colourRampInfectious != NULL) {
			delete colourRampInfectious;
			colourRampInfectious = NULL;
		}

		if (defaultFont != NULL) {
			al_destroy_font(defaultFont);
			defaultFont = NULL;
		}

		al_shutdown_ttf_addon();
		al_shutdown_font_addon();

	}

	return;
}
#endif

#if USE_VIDEO
void InterVideo::drawInfectionStateToRaster(ALLEGRO_BITMAP *drawTarget, bool bSetBlackAsAlpha, const Raster<int> *mask) {

	al_set_target_bitmap(landMapBmp);

	if (bSetBlackAsAlpha) {
		al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	} else {
		al_clear_to_color(al_map_rgb(0, 0, 0));
	}

	al_lock_bitmap(landMapBmp, ALLEGRO_LOCK_WRITEONLY, 0);
	//Use view window coordinates and zoom level to only draw relevant region:
	int maxX = min(int(viewX + ceil(screen_w / zoomLevel)), world->header->nCols);
	int maxY = min(int(viewY + ceil(screen_h / zoomLevel)), world->header->nRows);
	for (int rX = viewX; rX < maxX; rX++) {
		for (int rY = viewY; rY < maxY; rY++) {

			//if not masked out:
			if (mask == NULL || mask->getValue(rX, rY) >= 0) {
				if (world->locationArray[rX + rY*world->header->nCols] != NULL) {
					double dense;
					if (!bShowOnlySymptomatic) {
						dense = world->locationArray[rX + rY*world->header->nCols]->getInfectiousCoverageProportion(-1);
					} else {
						dense = world->locationArray[rX + rY*world->header->nCols]->getDetectableCoverageProportion(-1);
					}
					if (dense > 0.0) {
						al_put_pixel(rX - viewX, rY - viewY, al_map_rgb(colourRampInfectious->getRed(dense), colourRampInfectious->getGreen(dense), colourRampInfectious->getBlue(dense)));
					} else {
						double susceptibleCoverage = world->locationArray[rX + rY*world->header->nCols]->getSusceptibleCoverageProportion(-1);
						al_put_pixel(rX - viewX, rY - viewY, al_map_rgb(colourRampHealthy->getRed(susceptibleCoverage), colourRampHealthy->getGreen(susceptibleCoverage), colourRampHealthy->getBlue(susceptibleCoverage)));
					}
				}
			}

		}
	}

	al_unlock_bitmap(landMapBmp);

	//Draw the bitmap to the display:
	al_set_target_bitmap(drawTarget);
	if (bSetBlackAsAlpha) {
		al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	} else {
		al_clear_to_color(al_map_rgb(0, 0, 0));
	}
	al_draw_scaled_bitmap(landMapBmp, 0.0, 0.0, maxX, maxY, 0.0, 0.0, maxX*zoomLevel, maxY*zoomLevel, 0);

}
#endif

int InterVideo::intervene() {

#if USE_VIDEO
	if (enabled) {

		//Find most relevant mask raster:

		const Raster<int> *mask = NULL;
		double dMostRecentTime = world->timeStart - 1e9;

		if (masks.size() > 0) {
			//Find most recent mask to have taken effect
			for (auto const& maskIt : masks) {

				double maskTime = maskIt.first;

				//We need a tiny floating point tolerance here: if the next mask is just about to take effect, we should probably be using that
				if (maskTime <= (world->time + 0.01 * frequency) && maskTime > dMostRecentTime) {
					dMostRecentTime = maskTime;
					mask = &maskIt.second;
				}

			}
		}

		//Draw current epidemic status:
		drawInfectionStateToRaster(al_get_backbuffer(displayInfection), false, mask);

		//Move to drawing overlay onto display:

		//Draw overlay on display:
		//HACK: Draw a windsock:
		if (bDisplayWindSock) {
			//Poll the kernel to see what direction its going in:
			//TODO: Maybe poll lots of times to get an averaged kernel?
			DispersalKernel *pKernel = world->dispersal->getKernel(0);
			double targetX, targetY, dNorm;
			targetX = targetY = dNorm = 0.0;
			while (dNorm <= 0.0) {
				pKernel->KernelVirtualSporulation(world->time, world->header->nCols / 2.0, world->header->nRows / 2.0, targetX, targetY);
				dNorm = targetX*targetX + targetY*targetY;
			}

			//Get normalised direction:
			if (dNorm > 0.0) {
				dNorm = sqrt(dNorm);
				targetX /= dNorm;
				targetY /= dNorm;
			}

			//Draw an arrow:
			double boxSize = 100;
			double boxInset = 15;
			double boxCentreX = boxInset + boxSize / 2.0;
			double boxCentreY = screen_h - boxInset - boxSize + boxSize / 2.0;
			double boxThickness = 1;
			double arrowShaftLength = 0.5*boxSize*0.75;
			double arrowShaftThickness = 2;
			double arrowHeadLength = boxSize*0.1;


			ALLEGRO_COLOR white = al_map_rgb(255, 255, 255);

			//Put a box in the lower left corner of the screen:
			al_draw_rectangle(boxCentreX - boxSize / 2.0, boxCentreY - boxSize / 2.0, boxCentreX + boxSize / 2.0, boxCentreY + boxSize / 2.0, white, boxThickness);

			//Draw a line in the box from the centre in the direction of the wind:
			double arrowEndX = boxCentreX + arrowShaftLength*targetX;
			double arrowEndY = boxCentreY + arrowShaftLength*targetY;
			al_draw_line(boxCentreX, boxCentreY, arrowEndX, arrowEndY, white, arrowShaftThickness);

			//Put the arrowhead on:
			al_draw_filled_triangle(arrowEndX + arrowHeadLength*targetX, arrowEndY + arrowHeadLength*targetY, arrowEndX - arrowHeadLength*targetY, arrowEndY + arrowHeadLength*targetX, arrowEndX + arrowHeadLength*targetY, arrowEndY - arrowHeadLength*targetX, white);

			//Write "WIND" above the box:
			int textHeight = al_get_font_line_height(defaultFont);
			int textWidth = al_get_text_width(defaultFont, "WIND");
			al_draw_text(defaultFont, white, boxCentreX - textWidth / 2.0, boxCentreY - boxSize / 2.0 - textHeight*1.1, 0, "WIND");

		}

		if (bDisplayTime) {
			//Display the time on the screen:
			char szTime[256];
			sprintf(szTime, "Time: %11.6f", world->time);
			int textHeight = al_get_font_line_height(defaultFont);
			int textWidth = al_get_text_width(defaultFont, szTime);

			ALLEGRO_COLOR white = al_map_rgb(255, 255, 255);

			double boxInset = 15;//From above....
			al_draw_text(defaultFont, white, screen_w - textWidth - boxInset, screen_h - boxInset - textHeight*1.1, 0, szTime);

		}


		//Draw to display:
		al_set_target_bitmap(al_get_backbuffer(displayInfection));

		//Save image:
		if (bVideoExportImages) {
			char pngName[_N_MAX_STATIC_BUFF_LEN];

			sprintf(pngName, "%s%s_%.6f.%s", szPrefixOutput(), "INFECTIOUS_COVERAGE", world->time, "png");

			ALLEGRO_BITMAP *exportBitmap = al_get_backbuffer(displayInfection);

			if (bVideoExportNoHostAsAlpha) {

				//Get the old draw target
				ALLEGRO_BITMAP *oldDrawTarget = al_get_target_bitmap();

				//Export the current state:
				drawInfectionStateToRaster(outputBufferBmp, bVideoExportNoHostAsAlpha);



				//Set the export bitmap as our new target:
				exportBitmap = outputBufferBmp;

				//Restore the old draw target:
				al_set_target_bitmap(oldDrawTarget);

			}

			al_save_bitmap(pngName, exportBitmap);


		}

		//Limit FPS
		double frameDelay = 1.0 / FPSLimit - (clock_ms() - timeLastFrame) / 1000.0;
		//printk("FRAMEDELAY IS %f\n", frameDelay);
		if (frameDelay > 0.0) {
			al_rest(frameDelay);
		}

		//Display image:
		al_flip_display();

		timeLastFrame = clock_ms();

	}
#endif

	timeNext += frequency;
	return 1;
}

int InterVideo::finalAction() {

	//Output the final Video Frame
	//printf("DataDPC::finalAction()\n");

	if (enabled && !world->bGiveKeys) {

#if USE_VIDEO
		//Save image:
		if (bVideoExportLastframe) {
			char pngName[_N_MAX_STATIC_BUFF_LEN];

			sprintf(pngName, "%s%s_%.6f.%s", szPrefixOutput(), "INFECTIOUS_COVERAGE", world->time, "png");
			al_save_bitmap(pngName, al_get_backbuffer(displayInfection));
		}
#endif

	}

	return 1;
}

void InterVideo::writeSummaryData(FILE *pFile) {
	return;
}

#if USE_VIDEO
int InterVideo::writeRasterToFile(const char *title, Raster<int> *data, ColourRamp *ramp) {

	//TODO: Somewhere probably in allegro the ram is being trashed... causes crash on exit
	//problem is likely caused by writing outside the bitmap here despite checking this is not the case a dozen times
	if (enabled) {

		//DEBUG::

		//int xSize = 100;
		//int ySize = 100;

		//ALLEGRO_BITMAP *targetBMP = al_create_bitmap(xSize, ySize);

		////Draw current epidemic status:
		//al_lock_bitmap(targetBMP, ALLEGRO_LOCK_WRITEONLY, 0);
		//
		//al_set_target_bitmap(targetBMP);

		//al_clear_to_color(al_map_rgb(0, 0, 0));

		//for(int rX=0; rX < xSize; rX++) {
		//	for(int rY=0; rY < ySize; rY++) {
		//		al_put_pixel(rX, rY, al_map_rgb(200,100,000));
		//	}
		//}

		//al_unlock_bitmap(targetBMP);

		////Draw the bitmap to the file:
		//al_save_bitmap(title, targetBMP);

		//al_set_target_bitmap(NULL);

		//al_destroy_bitmap(targetBMP);

		//::DEBUG

		ALLEGRO_BITMAP *targetBMP = al_create_bitmap(data->header.nCols, data->header.nRows);

		//Draw current epidemic status:
		al_lock_bitmap(targetBMP, ALLEGRO_LOCK_WRITEONLY, 0);

		al_set_target_bitmap(targetBMP);

		al_clear_to_color(al_map_rgb(0, 0, 0));

		for (int rX = 0; rX < data->header.nCols; rX++) {
			for (int rY = 0; rY < data->header.nRows; rY++) {
				int iVal = data->getValue(rX, rY);
				al_put_pixel(rX, rY, al_map_rgb(ramp->getRed(iVal), ramp->getGreen(iVal), ramp->getBlue(iVal)));
			}
		}

		al_unlock_bitmap(targetBMP);

		//Draw the bitmap to the file:
		al_save_bitmap(title, targetBMP);

		//Seems need to do this otherwise get crash at shutdown for no good reason:
		al_set_target_bitmap(NULL);

		al_destroy_bitmap(targetBMP);
	}

	return enabled;
}
#endif
