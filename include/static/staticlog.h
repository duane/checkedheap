// -*- C++ -*-

/**
 * @file   staticlog.h
 * @brief  Statically returns the log (base 2) of a value.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Copyright (C) 2005 by Emery Berger, University of Massachusetts Amherst.
 */

#ifndef DH_STATICLOG_H
#define DH_STATICLOG_H

#include "staticif.h"

template <int Number>
class StaticLog;

template <>
class StaticLog<1> {
public:
  enum { VALUE = 0 };
};

template <>
class StaticLog<2> {
public:
  enum { VALUE = 1 };
};

template <int Number>
class StaticLog {
public:
  enum { VALUE = StaticIf<(Number > 1), StaticLog<Number/2>::VALUE + 1, 0>::VALUE };
};

#endif
