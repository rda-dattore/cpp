// FILE: satellite.h

#ifndef SATELLITE_H
#define   SATELLITE_H

#include <iodstream.hpp>
#include <datetime.hpp>

class SatelliteS
{
public:
  struct EarthCoordinate {
    EarthCoordinate() : lat(),lon() {}

    float lat,lon;
  };

  SatelliteS() {}
  virtual ~SatelliteS() {}
};

class InputSatelliteStream : public idstream
{
public:
  virtual int ignore()=0;
  virtual bool open(const char *filename)=0;
  virtual int peek()=0;
  virtual int read(unsigned char *buffer,size_t buffer_length)=0;
  virtual DateTime unpackDateTime(const unsigned char *buffer);
  DateTime getStartDateTime() const { return startDateTime; }
  DateTime getEndDateTime() const { return endDateTime; }
  std::string getID() const { return ID; }
  size_t getNumberOfScanLines() const { return numScanLines; }

protected:
  std::string ID;
  DateTime startDateTime,endDateTime;
  size_t numScanLines;
};

class SatelliteScanLine : public SatelliteS
{
public:
  SatelliteScanLine() : numPoints(0),coords(),subpoint(),dateTime(),width(0.),subpoint_resolution(0.),ascend_indicator(' ') {}
  virtual ~SatelliteScanLine() {}
  virtual void fill(const unsigned char *buffer,size_t dataTypeCode)=0;
  virtual DateTime unpackDateTime(const unsigned char *buffer)=0;
  unsigned char getAscendDescendIndicator() const { return ascend_indicator; }
  DateTime getDateTime() const { return dateTime; }
  std::vector<EarthCoordinate> getCoordinates() const { return coords; }
  size_t getNumberOfPoints() const { return numPoints; }
  float getResolutionAtSubPoint() const { return subpoint_resolution; }
  EarthCoordinate getSubPoint() const { return subpoint; }
  float getWidth() const { return width; }

protected:
  size_t numPoints;
  std::vector<EarthCoordinate> coords;
  EarthCoordinate subpoint;
  DateTime dateTime;
  float width,subpoint_resolution;
  unsigned char ascend_indicator;
};

class SatelliteImage : public SatelliteS
{
public:
  struct ImageCorners {
    ImageCorners() : sw_coord(),nw_coord(),ne_coord(),se_coord() {}

    EarthCoordinate sw_coord,nw_coord,ne_coord,se_coord;
  };
  SatelliteImage() : dateTime(),corners(),center_coord(),xres(0.),yres(0.) {}
  virtual ~SatelliteImage() {}
  virtual void fill(const unsigned char *buffer)=0;
  EarthCoordinate getCenter() const { return center_coord; }
  ImageCorners getCorners() const { return corners; }
  DateTime getDateTime() const { return dateTime; }
  float getXResolution() const { return xres; }
  float getYResolution() const { return yres; }

protected:
  DateTime dateTime;
  ImageCorners corners;
  EarthCoordinate center_coord;
  float xres,yres;
};

class InputNOAAPolarOrbiterDataStream : public InputSatelliteStream
{
public:
  int ignore();
  bool open(const char *filename);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  DateTime unpackDateTime(const unsigned char *buffer);
  short getDataTypeCode() const { return dataTypeCode; }

private:
  short dataTypeCode;
};

class NOAAPolarOrbiterSatelliteScanLine : public SatelliteScanLine
{
public:
  void fill(const unsigned char *buffer,size_t dataTypeCode);
  DateTime unpackDateTime(const unsigned char *buffer);
};

class InputMcIDASStream : public InputSatelliteStream
{
public:
  static const size_t NUM_SENSOR_SOURCES=231;
  static const char *sensorSources[NUM_SENSOR_SOURCES];
  int ignore();
  bool open(const char *filename);
  int peek();
  int read(unsigned char *buffer,size_t buffer_length);
  size_t getSensorNumber() const { return sensorNum; }

private:
  void swap(int& word);

  size_t sensorNum;
};

class McIDASImage : public SatelliteImage
{
public:
  McIDASImage() : nominalDateTime() {}
  virtual ~McIDASImage() {}
  void fill(const unsigned char *buffer);
  DateTime getNominalDateTime() const { return nominalDateTime; }

private:
  DateTime nominalDateTime;
  unsigned char bandMap[32];
};

#endif
