// Copyright (C) 2011 Technische Universitaet Muenchen
// This file is part of the preCICE project. For conditions of distribution and
// use, please see the license notice at http://www5.in.tum.de/wiki/index.php/PreCICE_License
#include "ImplicitCouplingScheme.hpp"
#include "impl/PostProcessing.hpp"
#include "impl/ConvergenceMeasure.hpp"
#include "Constants.hpp"
#include "mesh/SharedPointer.hpp"
#include "com/Communication.hpp"
#include "com/SharedPointer.hpp"
#include "io/TXTWriter.hpp"
#include "io/TXTReader.hpp"
#include "tarch/plotter/globaldata/TXTTableWriter.h"
#include <limits>

namespace precice {
namespace cplscheme {

tarch::logging::Log ImplicitCouplingScheme::
    _log("precice::cplscheme::ImplicitCouplingScheme" );

ImplicitCouplingScheme:: ImplicitCouplingScheme
(
  double                maxTime,
  int                   maxTimesteps,
  double                timestepLength,
  int                   validDigits,
  const std::string&    firstParticipant,
  const std::string&    secondParticipant,
  const std::string&    localParticipant,
  com::PtrCommunication communication,
  int                   maxIterations,
  constants::TimesteppingMethod dtMethod )
:
  BaseCouplingScheme(maxTime, maxTimesteps, timestepLength, validDigits),
  _firstParticipant(firstParticipant),
  _secondParticipant(secondParticipant),
  _doesFirstStep(false),
  _communication(communication),
  _iterationsWriter("iterations-" + localParticipant + ".txt"),
  //_residualWriterL1("residualL1-" + localParticipant + ".txt"),
  //_residualWriterL2("residualL2-" + localParticipant + ".txt"),
  //_amplificationWriter("amplification-" + localParticipant + ".txt"),
  _convergenceMeasures(),
  _postProcessing(),
  _extrapolationOrder(0),
  _maxIterations(maxIterations),
  _iterationToPlot(0),
  _timestepToPlot(0),
  _timeToPlot(0.0),
  _iterations(0),
  _totalIterations(0),
  _participantSetsDt(false),
  _participantReceivesDt(false),
  _hasToReceiveInitData(false),
  _hasToSendInitData(false)
{
  preciceCheck(_firstParticipant != _secondParticipant,
               "ImplicitCouplingScheme()", "First participant and "
               << "second participant must have different names!");
  if (dtMethod == constants::FIXED_DT){
    preciceCheck(not tarch::la::equals(timestepLength, UNDEFINED_TIMESTEP_LENGTH),
        "ImplicitCouplingScheme()", "Timestep length value has to be given "
        << "when the fixed timestep length method is chosen for an implicit "
        << "coupling scheme!");
  }
  if (localParticipant == _firstParticipant){
    _doesFirstStep = true;
    if (dtMethod == constants::FIRST_PARTICIPANT_SETS_DT){
      _participantSetsDt = true;
      setTimestepLength(UNDEFINED_TIMESTEP_LENGTH);
    }
  }
  else if (localParticipant == _secondParticipant){
    if (dtMethod == constants::FIRST_PARTICIPANT_SETS_DT){
      _participantReceivesDt = true;
    }
  }
  else {
    preciceError("initialize()", "Name of local participant \""
                 << localParticipant << "\" does not match any "
                 << "participant specified for the coupling scheme!");
  }
  preciceCheck((maxIterations > 0) || (maxIterations == -1),
               "ImplicitCouplingState()",
               "Maximal iteration limit has to be larger than zero!");
  assertion(_communication.use_count() > 0);
}

ImplicitCouplingScheme:: ~ImplicitCouplingScheme()
{}

void ImplicitCouplingScheme:: setExtrapolationOrder
(
  int order )
{
  preciceCheck((order == 0) || (order == 1) || (order == 2),
               "setExtrapolationOrder()", "Extrapolation order has to be "
               << " 0, 1, or 2!");
  _extrapolationOrder = order;
}

void ImplicitCouplingScheme:: addConvergenceMeasure
(
  int                         dataID,
  bool                        suffices,
  impl::PtrConvergenceMeasure measure )
{
  ConvergenceMeasure convMeasure;
  convMeasure.dataID = dataID;
  convMeasure.data = NULL;
  convMeasure.suffices = suffices;
  convMeasure.measure = measure;
  _convergenceMeasures.push_back(convMeasure);
}

void ImplicitCouplingScheme:: setIterationPostProcessing
(
  impl::PtrPostProcessing postProcessing )
{
  assertion(postProcessing.get() != NULL);
  _postProcessing = postProcessing;
}


void ImplicitCouplingScheme:: timestepCompleted()
{
  preciceTrace2("timestepCompleted()", getTimesteps(), getTime());
  preciceInfo("timestepCompleted()", "Timestep completed");
  setIsCouplingTimestepComplete(true);
  setTimesteps(getTimesteps() + 1 );
  //setTime(getTimesteps() * getTimestepLength() ); // Removes numerical errors
  if (isCouplingOngoing()){
    preciceDebug("Setting require create checkpoint");
    requireAction(constants::actionWriteIterationCheckpoint());
  }
}

void ImplicitCouplingScheme:: finalize()
{
   preciceTrace("finalize()");
   checkCompletenessRequiredActions();
   preciceCheck(isInitialized(), "finalize()",
                "Called finalize() before initialize()!");
   preciceCheck(not isCouplingOngoing(), "finalize()",
                "Called finalize() while isCouplingOngoing() returns true!");
}

void ImplicitCouplingScheme:: initializeTXTWriters()
{
  _iterationsWriter.addData("Timesteps", io::TXTTableWriter::INT );
  _iterationsWriter.addData("Total Iterations", io::TXTTableWriter::INT );
  _iterationsWriter.addData("Iterations", io::TXTTableWriter::INT );
  _iterationsWriter.addData("Convergence", io::TXTTableWriter::INT );
//  _residualWriterL1.addData("Iterations", io::TXTTableWriter::INT );
//  _residualWriterL2.addData ("Iterations", io::TXTTableWriter::INT );
//  _amplificationWriter.addData("Iterations", io::TXTTableWriter::INT );
//  _residualWriterL1.addData("L1-Residual", io::TXTTableWriter::DOUBLE );
//  _residualWriterL2.addData("L2-Residual", io::TXTTableWriter::DOUBLE );
//  _amplificationWriter.addData("Amplification" ,io::TXTTableWriter::DOUBLE );
//  int entries = 0;
//  if(getSendData().size() > 0 ){
//    entries = (int)getSendData().begin()->second.values->size();
//  }
//  int levels = 1;
//  int treatedEntries = 2;
//  int entriesCurrentLevel = 1;
//  while (treatedEntries < entries){
//    treatedEntries += entriesCurrentLevel;
//    levels ++;
//    entriesCurrentLevel *= 2;
//  }
//  if (treatedEntries == entries){
//    for (int i=0; i < levels; i++ ){
//      _residualWriterL1.addData("L1-Residual-level-"+i, io::TXTTableWriter::DOUBLE);
//      _residualWriterL2.addData("L2-Residual-level-"+i, io::TXTTableWriter::DOUBLE);
//      _amplificationWriter.addData("Amplification-level-"+i,io::TXTTableWriter::DOUBLE);
//    }
//  }
}

void ImplicitCouplingScheme:: setupDataMatrices(DataMap& data)
{
  preciceTrace("setupDataMatrices()");
  preciceDebug("Data size: " << data.size());
  // Reserve storage for convergence measurement of send and receive data values
  foreach (ConvergenceMeasure& convMeasure, _convergenceMeasures){
    assertion(convMeasure.data != NULL);
    if (convMeasure.data->oldValues.cols() < 1){
      convMeasure.data->oldValues.append(CouplingData::DataMatrix(
          convMeasure.data->values->size(), 1, 0.0));
    }
  }
  // Reserve storage for extrapolation of data values
  if (_extrapolationOrder > 0){
    foreach (DataMap::value_type& pair, data){
      int cols = pair.second->oldValues.cols();
      preciceDebug("Add cols: " << pair.first << ", cols: " << cols);
      assertion1(cols <= 1, cols);
      pair.second->oldValues.append(CouplingData::DataMatrix(
          pair.second->values->size(), _extrapolationOrder + 1 - cols, 0.0));
    }
  }
}

void ImplicitCouplingScheme:: setupConvergenceMeasures()
{
  preciceTrace("setupConvergenceMeasures()");
  assertion(not _doesFirstStep);
  preciceCheck(not _convergenceMeasures.empty(), "setupConvergenceMeasures()",
      "At least one convergence measure has to be defined for "
      << "an implicit coupling scheme!");
  foreach (ConvergenceMeasure& convMeasure, _convergenceMeasures){
    int dataID = convMeasure.dataID;
    if ((getSendData(dataID) != NULL)){
      convMeasure.data = getSendData(dataID);
    }
    else {
      convMeasure.data = getReceiveData(dataID);
      assertion(convMeasure.data != NULL);
    }
  }
}

bool ImplicitCouplingScheme:: measureConvergence()
{
  preciceTrace("measureLocalConvergence()");
  using boost::get;
  bool allConverged = true;
  bool oneSuffices = false;
  assertion(_convergenceMeasures.size() > 0);
  foreach(ConvergenceMeasure& convMeasure, _convergenceMeasures){
    assertion(convMeasure.data != NULL);
    assertion(convMeasure.measure.get() != NULL);
    utils::DynVector& oldValues = convMeasure.data->oldValues.column(0);
    preciceDebug("old values in convergence "<< oldValues);
    convMeasure.measure->measure(oldValues, *convMeasure.data->values);
    if (not convMeasure.measure->isConvergence()){
      //preciceDebug("Local convergence = false");
      allConverged = false;
    }
    else if (convMeasure.suffices == true){
      oneSuffices = true;
    }
    preciceInfo("measureConvergence()", convMeasure.measure->printState());
  }
  if (allConverged){
    preciceInfo("measureConvergence()", "All converged");
  }
  else if (oneSuffices){
    preciceInfo("measureConvergence()", "Sufficient measure converged");
  }
  return allConverged || oneSuffices;
}

void ImplicitCouplingScheme:: extrapolateData(DataMap& data)
{
   preciceTrace("extrapolateData()");
   bool startWithFirstOrder = (getTimesteps() == 1) && (_extrapolationOrder == 2);
   if((_extrapolationOrder == 1) || startWithFirstOrder ){
      preciceInfo("extrapolateData()", "Performing first order extrapolation" );
      foreach(DataMap::value_type & pair, data ){
         preciceDebug("Extrapolate data: " << pair.first);
         assertion(pair.second->oldValues.cols() > 1 );
         utils::DynVector & values = *pair.second->values;
         pair.second->oldValues.column(0) = values;    // = x^t
         values *= 2.0;                                  // = 2 * x^t
         values -= pair.second->oldValues.column(1);   // = 2*x^t - x^(t-1)
         pair.second->oldValues.shiftSetFirst(values );
      }
   }
   else if(_extrapolationOrder == 2 ){
      preciceInfo("extrapolateData()", "Performing second order extrapolation" );
      foreach(DataMap::value_type & pair, data ) {
         assertion(pair.second->oldValues.cols() > 2 );
         utils::DynVector & values = *pair.second->values;
         pair.second->oldValues.column(0) = values;        // = x^t                                     // = 2.5 x^t
//         utils::DynVector & valuesOld1 = pair.second.oldValues.getColumn(1);
//         utils::DynVector & valuesOld2 = pair.second.oldValues.getColumn(2);
//         for(int i=0; i < values.size(); i++ ) {
//            values[i] -= valuesOld1[i] * 3.0; // =
//            values[i] += valuesOld2[i] * 3.0; // =
//         }
         values *= 2.5;                                      // = 2.5 x^t
         utils::DynVector & valuesOld1 = pair.second->oldValues.column(1);
         utils::DynVector & valuesOld2 = pair.second->oldValues.column(2);
         for(int i=0; i < values.size(); i++ ){
            values[i] -= valuesOld1[i] * 2.0; // = 2.5x^t - 2x^(t-1)
            values[i] += valuesOld2[i] * 0.5; // = 2.5x^t - 2x^(t-1) + 0.5x^(t-2)
         }
         pair.second->oldValues.shiftSetFirst(values );
         //preciceDebug("extrapolateData()", "extrapolated data to \""
         //               << *pair.second.values );
      }
   }
   else {
      preciceError("extrapolateData()", "Called extrapolation with order != 1,2!" );
   }
}

void ImplicitCouplingScheme:: newConvergenceMeasurements()
{
   preciceTrace("newConvergenceMeasurements()");
   foreach (ConvergenceMeasure& convMeasure, _convergenceMeasures){
     assertion(convMeasure.measure.get() != NULL);
     convMeasure.measure->newMeasurementSeries();
   }
}

std::vector<std::string> ImplicitCouplingScheme:: getCouplingPartners() const
{
  std::vector<std::string> partnerNames;

  // Add non-local participant
  if(_doesFirstStep){
    partnerNames.push_back(_secondParticipant);
  }
  else {
    partnerNames.push_back(_firstParticipant);
  }
  return partnerNames;
}

void ImplicitCouplingScheme:: sendState
(
  com::PtrCommunication communication,
  int                   rankReceiver )
{
  preciceTrace1("sendState()", rankReceiver);
  communication->startSendPackage(rankReceiver );
  BaseCouplingScheme::sendState(communication, rankReceiver );
  communication->send(_maxIterations, rankReceiver );
  communication->send(_iterations, rankReceiver );
  communication->send(_totalIterations, rankReceiver );
  communication->finishSendPackage();
}

void ImplicitCouplingScheme:: receiveState
(
  com::PtrCommunication communication,
  int                   rankSender )
{
  preciceTrace1("receiveState()", rankSender);
  communication->startReceivePackage(rankSender);
  BaseCouplingScheme::receiveState(communication, rankSender);
  communication->receive(_maxIterations, rankSender);
  int subIteration = -1;
  communication->receive(subIteration, rankSender);
  _iterations = subIteration;
  communication->receive(_totalIterations, rankSender);
  communication->finishReceivePackage();
}

std::string ImplicitCouplingScheme:: printCouplingState() const
{
  std::ostringstream os;
  os << "it " << _iterationToPlot; //_iterations;
  if(_maxIterations != -1 ){
    os << " of " << _maxIterations;
  }
  os << " | " << printBasicState(_timestepToPlot, _timeToPlot) << " | " << printActionsState();
  return os.str();
}

void ImplicitCouplingScheme:: exportState
(
  const std::string& filenamePrefix ) const
{
  if (not _doesFirstStep){
    io::TXTWriter writer(filenamePrefix + "_cplscheme.txt");
    foreach (const BaseCouplingScheme::DataMap::value_type& dataMap, getSendData()){
      writer.write(dataMap.second->oldValues);
    }
    foreach (const BaseCouplingScheme::DataMap::value_type& dataMap, getReceiveData()){
      writer.write(dataMap.second->oldValues);
    }
    if (_postProcessing.get() != NULL){
      _postProcessing->exportState(writer);
    }
  }
}

void ImplicitCouplingScheme:: importState
(
  const std::string& filenamePrefix )
{
  if (not _doesFirstStep){
    io::TXTReader reader(filenamePrefix + "_cplscheme.txt");
    foreach (BaseCouplingScheme::DataMap::value_type& dataMap, getSendData()){
      reader.read(dataMap.second->oldValues);
    }
    foreach (BaseCouplingScheme::DataMap::value_type& dataMap, getReceiveData()){
      reader.read(dataMap.second->oldValues);
    }
    if (_postProcessing.get() != NULL){
      _postProcessing->importState(reader);
    }
  }
}

//void ImplicitCouplingScheme:: writeResidual
//(
//  const utils::DynVector& values,
//  const utils::DynVector& oldValues )
//{
//  using namespace tarch::la;
//  size_t entries = (size_t)values.size();
//
//  // Compute current residuals
//  utils::DynVector residual(values );
//  residual -= oldValues;
//  utils::DynVector oldValuesTemp(oldValues );
//
//  size_t levels = 1;
//  size_t treatedEntries = 2;
//  size_t entriesCurrentLevel = 1;
//  while(treatedEntries < entries ){
//    treatedEntries += entriesCurrentLevel;
//    levels ++;
//    entriesCurrentLevel *= 2;
//  }
//
//  double residualNormL1 = norm1(residual );
//  double residualNormL2 = norm2(residual );
//  double amplification = norm2(values) / norm2(oldValues);
//  utils::DynVector hierarchicalResidualNormsL1;
//  utils::DynVector hierarchicalResidualNormsL2;
//  utils::DynVector hierarchicalAmplification;
//
//  _residualWriterL1.writeData("L1-Residual", residualNormL1 );
//  _residualWriterL2.writeData("L2-Residual", residualNormL2 );
//  _amplificationWriter.writeData("Amplification", amplification );
//
//  if(treatedEntries == entries ){ // Hierarchizable
//    treatedEntries = 2;
//    hierarchicalResidualNormsL1.append(utils::DynVector(levels, 0.0) );
//    hierarchicalResidualNormsL2.append(utils::DynVector(levels, 0.0) );
//    hierarchicalAmplification.append(utils::DynVector(levels, 0.0) );
//    entriesCurrentLevel = std::pow(2.0, (int)(levels - 2));
//    for(size_t level=levels-1; level > 0; level-- ){
//      size_t stepsize = (entries - 1) / std::pow(2.0, (int)(level-1));
//      assertion(stepsize % 2 == 0 );
//      size_t index = stepsize / 2;
//
//      double amplificationNom = 0.0;
//      double amplificationDenom = 0.0;
//      for(size_t i=0; i < entriesCurrentLevel; i++ ){
//        residual[index] -=(residual[index - stepsize/2] +
//                             residual[index + stepsize/2] ) / 2.0;
//        oldValuesTemp[index] -=(oldValuesTemp[index - stepsize/2] +
//                                  oldValuesTemp[index + stepsize/2] ) / 2.0;
//        hierarchicalResidualNormsL1[level] += std::abs(residual[index] );
//        hierarchicalResidualNormsL2[level] += residual[index] * residual[index];
//        amplificationNom += std::pow(residual[index] + oldValuesTemp[index], 2 );
//        amplificationDenom += std::pow(oldValuesTemp[index], 2 );
//        index += stepsize;
//      }
//      hierarchicalResidualNormsL2[level] = std::sqrt(hierarchicalResidualNormsL2[level] );
//      hierarchicalAmplification[level] = std::sqrt(amplificationNom) / std::sqrt(amplificationDenom);
//      treatedEntries += entriesCurrentLevel;
//      entriesCurrentLevel /= 2;
//    }
//    hierarchicalResidualNormsL1[0] =
//        std::abs(residual[0]) + std::abs(residual[entries-1] );
//    hierarchicalResidualNormsL2[0] = std::sqrt(
//        residual[0] * residual[0] + residual[entries-1] * residual[entries-1] );
//    hierarchicalAmplification[0] =
//        std::sqrt(std::pow(values[0], 2) + std::pow(values[entries-1], 2)) /
//        std::sqrt(std::pow(oldValues[0], 2) + std::pow(oldValues[entries-1], 2));
//    for(size_t i=0; i < levels; i++ ){
//      _residualWriterL1.writeData("L1-Residual-level-" + i, residualNormL1 );
//      _residualWriterL2.writeData("L2-Residual-level-" + i, residualNormL2 );
//      _amplificationWriter.writeData("Amplification-level-" + i, amplification );
//    }
//  }
//}

}} // namespace precice, cplscheme