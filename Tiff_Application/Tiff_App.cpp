//============================================================================
// Name        : Tiff_Reporter.cpp
// Author      : Ricardo O. Mitchell
// Version     : 2020.5.17
// Copyright   : Free to reuse in whole or in part for personal or comercial
//				 applications the only condition being that you plug my authorship :)
//
// Description : Tiff Reporter in C++. A simple program to read mutiple tiff files
//				 and output all the metadata of the file (not including the actual image(s))
//============================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <bitset>
#include <unordered_map>
#include <vector>
#include <cmath>
using namespace std;

int charToInt(char *, int, int);
char* readBytes(fstream &, int, char*, int);
int getFirstIFDOffset(fstream &);
int getNextIFDOffset(fstream &, int);
void setEndianess(fstream &);
int getIFDEntryCount(fstream &, int);
void getIFDInfo(fstream &, unordered_map<int, string> &, int, int);
void printFileInfo();

const int bytesPerIFDEntry = 12;
const int offsetsReadSize = 128; //TODO: No research done yet for an ideal size
const string typeNames[13] = {"UNDEFINED","BYTE","ASCII","SHORT","LONG","RATIONAL","SBYTE","UNDEFINED","SSHORT","SLONG","SRATIONAL","FLOAT","DOUBLE"}; //holds byte sizes for each data type
const int typeBytes[13] = {0,1,1,2,4,8,1,1,2,4,8,4,8}; //holds byte sizes for each data type
const int littleEndianCode = 18761; // i.e. 0x4949 or char 'II'
bool isLittleEndian = false;
vector <vector<pair<string, vector<int>>>> allIFDs;

//TODO: take as program run arguments or user prompted input
vector<string> fileNames{"pics-3.8.0\\libtiffpic\\strike.tif", "pics-3.8.0\\libtiffpic\\multipage_tiff_example.tif", "pics-3.8.0\\libtiffpic\\zackthecat.tif"};
string outputFile = "Tiff_App_Output.txt";

int main()
{
	unordered_map<int, string> tagsMap;//TODO: transfer this and initialization below to header file

	tagsMap[254] = "NewSubfileType";
	tagsMap[255] = "SubfileType";
	tagsMap[256] = "ImageWidth";
	tagsMap[257] = "ImageLength";
	tagsMap[258] = "BitsPerSample";
	tagsMap[259] = "Compression";
	tagsMap[262] = "PhotometricInterpretation";
	tagsMap[263] = "Threshholding";
	tagsMap[264] = "CellWidth";
	tagsMap[265] = "CellLength";
	tagsMap[266] = "FillOrder";
	tagsMap[269] = "DocumentName";
	tagsMap[270] = "ImageDescription";
	tagsMap[271] = "Make";
	tagsMap[272] = "Model";
	tagsMap[273] = "StripOffsets";
	tagsMap[274] = "Orientation";
	tagsMap[277] = "SamplesPerPixel";
	tagsMap[278] = "RowsPerStrip";
	tagsMap[279] = "StripByteCounts";
	tagsMap[280] = "MinSampleValue";
	tagsMap[281] = "MaxSampleValue";
	tagsMap[282] = "XResolution";
	tagsMap[283] = "YResolution";
	tagsMap[284] = "PlanarConfiguration";
	tagsMap[285] = "PageName";
	tagsMap[286] = "XPosition";
	tagsMap[287] = "YPosition";
	tagsMap[288] = "FreeOffsets";
	tagsMap[289] = "FreeByteCounts";
	tagsMap[290] = "GrayResponseUnit";
	tagsMap[291] = "GrayResponseCurve";
	tagsMap[292] = "T4Options";
	tagsMap[293] = "T6Options";
	tagsMap[296] = "ResolutionUnit";
	tagsMap[297] = "PageNumber";
	tagsMap[301] = "TransferFunction";
	tagsMap[305] = "Software";
	tagsMap[306] = "DateTime";
	tagsMap[315] = "Artist";
	tagsMap[316] = "HostComputer";
	tagsMap[317] = "Predictor";
	tagsMap[318] = "WhitePoint";
	tagsMap[319] = "PrimaryChromaticities";
	tagsMap[320] = "ColorMap";
	tagsMap[321] = "HalftoneHints";
	tagsMap[322] = "TileWidth";
	tagsMap[323] = "TileLength";
	tagsMap[324] = "TileOffsets";
	tagsMap[325] = "TileByteCounts";
	tagsMap[332] = "InkSet";
	tagsMap[333] = "InkNames";
	tagsMap[334] = "NumberOfInks";
	tagsMap[336] = "DotRange";
	tagsMap[337] = "TargetPrinter";
	tagsMap[338] = "ExtraSamples";
	tagsMap[339] = "SampleFormat";
	tagsMap[340] = "SMinSampleValue";
	tagsMap[341] = "SMaxSampleValue";
	tagsMap[342] = "TransferRange";
	tagsMap[512] = "JPEGProc";
	tagsMap[513] = "JPEGInterchangeFormat";
	tagsMap[514] = "JPEGInterchangeFormatLngth";
	tagsMap[515] = "JPEGRestartInterval";
	tagsMap[517] = "JPEGLosslessPredictors";
	tagsMap[518] = "JPEGPointTransforms";
	tagsMap[519] = "JPEGQTables";
	tagsMap[520] = "JPEGDCTables";
	tagsMap[521] = "JPEGACTables";
	tagsMap[529] = "YCbCrCoefficients";
	tagsMap[530] = "YCbCrSubSampling";
	tagsMap[531] = "YCbCrPositioning";
	tagsMap[532] = "ReferenceBlackWhite";

	int filesCount = 0;

	for(auto& fileName: fileNames)
	{
		fstream tiffImage(fileName, ios::in | ios::out | ios::ate | ios::binary);
		
		if(tiffImage.is_open())
		{
			vector<pair<string, vector<int>>> fileIFDsBelongTo;
			vector<int> fileNameIndex{filesCount};

			fileIFDsBelongTo.push_back(make_pair(fileName,fileNameIndex));
			allIFDs.push_back(fileIFDsBelongTo);

			setEndianess(tiffImage);

			int IFDOffset = getFirstIFDOffset(tiffImage);

			int numOFIFDEntries = getIFDEntryCount(tiffImage, IFDOffset);

			while(IFDOffset != 0)
			{
				IFDOffset += 2;
				getIFDInfo(tiffImage, tagsMap, IFDOffset, numOFIFDEntries);
				IFDOffset = getNextIFDOffset(tiffImage, (IFDOffset + (bytesPerIFDEntry * numOFIFDEntries)));
				numOFIFDEntries = getIFDEntryCount(tiffImage, IFDOffset);
			}

		}
		else
		{
			cerr << "Error opening file " << fileName << endl;
		}

		tiffImage.close(); //TODO:closing here means reopening files for manipulation, better way required later
		filesCount++;
	}

	printFileInfo();

	return 0;
}

//fstream reads as only char, this converts it to integer values
int charToInt(char *readBytes, int start, int count)
{
	string binary;

	for(int i = start; i < start + count; i++)
	{
		bitset<8> a (readBytes[i]); //converts char to binary
		binary = isLittleEndian ? a.to_string() + binary : binary + a.to_string(); //binary to int in order of endianess
	}

	return stoi(binary, 0, 2);
}

/*
int charToFloat(char *readBytes, int start, int count)
{
	string binary;
	int power = 2;
	int count = 0;
	int numerator = 0;
	int denominator = 0;
	int exponent = 0;
	bool negative = false;
	bool denominatorFound = false;

		//bitset<8> a (readBytes[i]); //converts char to binary
		//binary = isLittleEndian ? a.to_string() + binary : binary + a.to_string(); //binary to int in order of endianess
			if(readBytes[start] == 1)
			{
				negative = true;
			}

			exponent = charToInt(readBytes, 1, 8) - 127;

			for(int i = (start + count); i != (start -1); i--)
			{
				if(readBytes[i] == 1)
				{
					if(denominatorFound == false)
					{
						denominatorFound = true;
						denominator = pow(2, (count + 1)); //TODO: put correct value here
					}
				}
			}

	return stoi(binary, 0, 2);
}
*/

int getFirstIFDOffset(fstream &tiffImage)
{
	char tiffField[4];
	return charToInt(readBytes(tiffImage, 4, tiffField, 4), 0,4);
}

//determines and set byte order
void setEndianess(fstream &tiffImage)
{
	char tiffField[2];
	isLittleEndian = (charToInt(readBytes(tiffImage, 0, tiffField, 2), 0,2) == littleEndianCode) ? true : false;
}

int getNextIFDOffset(fstream &tiffImage, int offset)
{
	char tiffField[4];
	return charToInt(readBytes(tiffImage, offset, tiffField, 4), 0,4);
}

int getIFDEntryCount(fstream &tiffImage, int offset)
{
	char tiffField[2];
	return charToInt(readBytes(tiffImage, offset, tiffField, 2), 0,2);
}

//gets and stores all tags/fields along with associated data
void getIFDInfo(fstream &tiffImage, unordered_map<int, string> &tagLookUp, int offset, int NumOfIFDEntries)
{
	vector<pair<string, vector<int>>> IFDTags; // tags and (meta) data

	for(int i = offset; i < (offset + bytesPerIFDEntry * NumOfIFDEntries); i += bytesPerIFDEntry)
	{
		vector<int> tagInfo;
		char tiffField[bytesPerIFDEntry]; // hold a single IFD entry i.e. a tag/field along with metadata and value

		tiffImage.seekg(i, ios::beg);
		tiffImage.read(tiffField, bytesPerIFDEntry);
		string tagName;
		
		try
		{
			tagName = tagLookUp.at(charToInt(tiffField, 0, 2));
		}
		catch(const std::exception& e)
		{
			tagName = "Undefined-" + to_string(charToInt(tiffField, 0, 2));
		}
		
		tagInfo.push_back(charToInt(tiffField, 2, 2)); // tag type
		tagInfo.push_back(charToInt(tiffField, 4, 4)); // number of elements

		int valueStartPosition = 8; // values begin at byte 8 for each IFD entry
		int valByteSize = typeBytes[tagInfo[0]];
		int numOfVals = tagInfo[1]; // value can be an array
		char offsetVals[offsetsReadSize]; // if tag's value was an offset this needed for subsequent reads to grab value(s) at the offset
		int totalValSize = valByteSize * numOfVals;
		int iterateBy = valByteSize != 8 ? valByteSize : 4; //rational types will be split into 2 parts of 4 bytes each

		// If value(s) bigger than 4 bytes then it's an offset (eg. rational types which are 8 bytes) so register it, advance
		// to offset, and grab the data.
		// Rational types has 1st 4 bytes as numerator and 2nd 4 bytes as denominator
		// therefore both numerator and denominator can be stored as an int type
		if(totalValSize > 4)
		{
			int offset = charToInt(tiffField, 8, 4);
			int count;

			tagInfo.push_back(offset);			
			tiffImage.seekg(offset, ios::beg);

			valueStartPosition = 0; // now at the offset and no longer reading within the IFD's entry's 12 bytes...so start from the top for values

			for(int j = 0; j < totalValSize; j += offsetsReadSize) //loops if totalValSize > offsetReadSize
			{
				count = ((j + offsetsReadSize) > totalValSize)  ? totalValSize % offsetsReadSize : offsetsReadSize; //last read might be < offsetsReadmax

				tiffImage.read(offsetVals, count);

				for(int k = valueStartPosition; k < count; k += iterateBy) // grab data at each offset
				{
					tagInfo.push_back(charToInt(offsetVals, k, iterateBy));
				}
			}
		}
		else // not an offset so grab data from tag's 4 byte value field
		{
			for(int k = 0; k < totalValSize; k += iterateBy)
			{
				tagInfo.push_back(charToInt(tiffField, valueStartPosition, iterateBy));
				valueStartPosition += iterateBy;
			}
		}
		IFDTags.push_back(make_pair(tagName,tagInfo)); // add entry to IFD
	}

	allIFDs.push_back(IFDTags); // add current IFD to list of all IFDs
}

char* readBytes(fstream &tiffImage, int offset, char* tiffField, int arraySize) // grabs a specific amount of data from a specific location
{
	tiffImage.seekg(offset, ios::beg);

	tiffImage.read(tiffField, arraySize);

	return tiffField;
}

void printFileInfo() // prints all tiff files info including tags and values
{
	ofstream tiffAppOutput(outputFile);

	if(tiffAppOutput.is_open())
	{
		int test = 73;
		tiffAppOutput << static_cast<char>(test) << "\n\n\n";
		for(auto& i: allIFDs) // grabs an IFD
		{
			for(auto& j: i) // grabs an IFD entry/tag/field
			{
				tiffAppOutput << j.first << "\t"; // tag or file name

				if(j.second.size() == 1) // no data for file names so skip data extraction
				{
					tiffAppOutput << endl; // tag or file name
					continue;
				}

				int count = 1; // tracks depth of field data
				int separateCounter = 1; // helps splitting up data accross multiple lines for better presentation.
				int typeSize;
				int typeIndex;
				int valCount; // how man values expected
				int rationalCount = 1; // used to track or alternate between numerator and denominator for rational values
				bool isRational = false;

				for(auto& k: j.second) // grabs tag info
				{
					switch (count)
					{
						case 1:
							tiffAppOutput << "Type: " << typeNames[k] << "\t\t";
							typeIndex = k; // used later to determine if type is ascii
							typeSize = typeBytes[typeIndex];
							isRational = (k == 5 || k == 10) ? true : false;
							break;
						case 2:
							tiffAppOutput << "Count: " << k << "\t\tValue(s):" << endl;
							valCount = isRational ? k * 2 : k; // rational values are split into two parts thus doubling the count
							break;
						case 3:
							if ((typeSize * valCount) <= 4) // print only if value wasn't an offset
							{
								if(valCount == 1) // and new line if it's only a single value
								{
									if(typeIndex == 2) // check if value is ASCII
									{
										tiffAppOutput << static_cast<char>(k) << endl;
									}
									else
									{
										tiffAppOutput << k << endl;
									}									
								}
								else // there are multiple values so add tab to allow next value to be on same line if possible
								{
									if(typeIndex == 2) // check if value is ASCII
									{
										tiffAppOutput << static_cast<char>(k) << "\t";
									}
									else
									{
										tiffAppOutput << k << "\t";
									}

									separateCounter++;
								}
							}
							break;
						default:
							if(isRational) // handles special format of rational numbers i.e. presented as fractions eg. 23/45
							{
								tiffAppOutput << k;

								if(rationalCount == 1) // value is the numerato so add "\" after it to represent a fraction
								{
									tiffAppOutput << "\\";
									rationalCount = 2;
								}
								else // value is the denominator so add tab to allow for another rational value to be presented on the same line
								{
									tiffAppOutput << "\t";
									rationalCount = 1;
								}
							}
							else // not rational value so print the value along with tab to allow another value to be presented on the same line
							{
								if(typeIndex == 2) // check if value is ASCII
								{
									tiffAppOutput << static_cast<char>(k) << "\t";
								}
								else
								{
									tiffAppOutput << k << "\t";
								}
								
							}

							if ((separateCounter % 20) == 0 || separateCounter == valCount) // go to new line if too many values on the same line or last value printed
							{
								tiffAppOutput << endl;
							}

							separateCounter++;
							break;
					}
					count++;
				}

				tiffAppOutput << endl;
			}
			tiffAppOutput << endl;
		}
}
else
{
	cerr << "Unable to open output file " << outputFile;
}

}
