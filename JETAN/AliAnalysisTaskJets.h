#ifndef ALIANALYSISTASKJETS_H
#define ALIANALYSISTASKJETS_H
 
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */
 
#include "AliAnalysisTaskSE.h"
class AliJetFinder;
class AliESDEvent;
class TTree;
class AliAODEvent;
class AliJetHistos;
class TClonesArray;


class AliAnalysisTaskJets : public AliAnalysisTaskSE
{
 public:
    AliAnalysisTaskJets();
    AliAnalysisTaskJets(const char* name);
    virtual ~AliAnalysisTaskJets() {;}
    // Implementation of interface methods
    virtual void UserCreateOutputObjects();
    virtual void Init();
    virtual void LocalInit() {Init();}
    virtual void UserExec(Option_t *option);
    virtual void Terminate(Option_t *option);
 private:
  AliAnalysisTaskJets(const AliAnalysisTaskJets &det);
  AliAnalysisTaskJets &operator=(const AliAnalysisTaskJets &det);
    
 private:
    AliJetFinder* fJetFinder;    //  Pointer to the jet finder 
    TTree*        fTree;         //! The input tree
    AliJetHistos* fHistos;       //  Histogram manager class
    TList*        fListOfHistos; //  Output list of histograms
    TClonesArray* fNewJets;
    
    ClassDef(AliAnalysisTaskJets, 2); // Analysis task for standard jet analysis
};
 
#endif
