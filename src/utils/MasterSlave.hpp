// Copyright (C) 2011 Technische Universitaet Muenchen
// This file is part of the preCICE project. For conditions of distribution and
// use, please see the license notice at http://www5.in.tum.de/wiki/index.php/PreCICE_License
#ifndef PRECICE_UTILS_MASTER_SLAVE_HPP_
#define PRECICE_UTILS_MASTER_SLAVE_HPP_

#include "Dimensions.hpp"

#include "com/Communication.hpp"

#include "tarch/logging/Log.h"

namespace precice {
namespace utils {

/**
 * @brief Utility class for managing Master-Slave operations.
 */
class MasterSlave
{
public:

  static int _rank;
  static int _size;
  static int _masterRank;

  /**
   * @brief True if this process is running the master.
   */
  static bool _masterMode;
  /**
   * @brief True if this process is running a slave.
   */
  static bool _slaveMode;

  /**
   * @brief Communication between the master and all slaves.
   */
  static com::Communication::SharedPointer _communication;


  /**
   * @brief Configure the master-slave communication.
   */
  static void configure(int rank, int size);

  /**
   * @brief the l2 norm of a vector is calculated on distributed data.
   */
  static double l2norm(const DynVector& vec);
  static double l2norm(const EigenVector& vec);

  /**
   * @brief the wrms norm of a vector is calculated on distributed data.
   */
  static double wrmsNorm(const DynVector& vec, const DynVector& weights);
  static double wrmsNorm(const EigenVector& vec, const EigenVector& weights);

  /**
   * @brief the dot product of 2 vectors is calculated on distributed data.
   */
  static double dot(const DynVector& vec1, const DynVector& vec2);
  static double dot(const EigenVector& vec1, const EigenVector& vec2);

  /**
   * @brief the wrms dot product of 2 vectors is calculated on distributed data.
   */
  //static double wrmsDot(const DynVector& vec1, const DynVector& vec2, const DynVector& weights);
  //static double wrmsDot(const EigenVector& vec1, const EigenVector& vec2, const EigenVector& weights);

  static void reset();

  static void broadcast(bool& value);

  static void broadcast(double& value);
  
  static void broadcast(double* values, int size);

private:

  static tarch::logging::Log _log;

};


}} // namespace precice, utils

#endif /* PRECICE_UTILS_MASTER_SLAVE_HPP_ */
