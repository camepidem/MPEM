// RasterHeader.cpp

#include "stdafx.h"
#include "RasterHeader.h"

template <> const int RasterHeader<int>::NODATA_default = -1;
template <> const double RasterHeader<double>::NODATA_default = -1.0;
template <> const std::string RasterHeader<std::string>::NODATA_default = "NA";
