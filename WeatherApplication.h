//WeatherApplication.h

#pragma once

class WeatherApplication {

public:

	WeatherApplication();
	~WeatherApplication();

	int init(string sWeatherFileName, int bFixColumnSelection, WeatherDatabase &weatherDB);

	double dGetTimeOfNextWeatherEvent();

	void applyWeatherEvent(double dCurrentTime, WeatherDatabase &weatherDB);

	void reset();

protected:

	class WeatherApplicationElement {

	public:

		WeatherApplicationElement(string weatherApplicationElementLine, WeatherDatabase &weatherDB) {
			
			//Line format:
			//Time [Targets ...] files ...

			//Remove any leading/trailing whitespace:
			trim(weatherApplicationElementLine);

			vector<string> fields;
			split(fields, weatherApplicationElementLine, "[]");


			if(fields.size() != 3) {
				reportReadError("Did not find correct formatting in weather line: Time [Target1 Target2 ...] File1 File2 ...\nRead: \"%s\"\n", weatherApplicationElementLine.c_str());
			} else {

				stringstream ssTime(fields[0]);
				//Time:
				if(!(ssTime >> dTime)) {
					reportReadError("Unable to parse time from weather file contents: %s\n", fields[0].c_str());
				}

				//Targets:
				vector<string> targets;
				split(targets, fields[1], ", ");

				for(size_t iTarget=0; iTarget<targets.size(); iTarget++) {
					vTargets.push_back(PopModPair(targets[iTarget]));
				}

				//Files:
				split(vFileIDs, fields[2], " ", splitTypes::no_empties);

				//Report file to database:
				for(size_t iFile=0; iFile<vFileIDs.size(); iFile++) {
					if(!vFileIDs[iFile].empty()) {
						weatherDB.addFileToQueue(vFileIDs[iFile]);
					} else {
						vFileIDs.erase(vFileIDs.begin()+iFile);
						iFile--;
					}
				}

			}

		}

		void apply(WeatherDatabase &weatherDB, size_t iCol) {

			weatherDB.applyFileIDtoProperty(vFileIDs[iCol], vTargets);

		}

		double dTime;
		vector<PopModPair> vTargets;
		vector<string> vFileIDs;

	};

	vector<WeatherApplicationElement> weatherElements;

	size_t iNextApplicationElement;

	int bFixedColSelection;
	size_t iWeatherCol;

	size_t generateWeatherCol(size_t iElement);

};
