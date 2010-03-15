/**************************************************************************
 * Copyright(c) 1998-2007, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
////////////////////////////////////////////////////////////////////////////
//  Class to perform pt-spectra (and ptMin-spectra) extraction of mothers //
//  particles starting from measured pt-spectra of daughter particle      //
//  that come from inclusive decays.                                      // 
//  E.g.: B->J/psi+X , B->e+X, B->D0+X, etc.                              //
//                                                                        //
//  In order to use this class, one first has to run a simulation         // 
//  (only kinematics)  of the decay channel under study. Standard input   //
//  of this class is therefore the file galice.root. In order             //
//  to speed up, a method exhists which reads the Kinematics.root         // 
//  and produces an ntupla with just the particles of the decay under     //
//  study                                                                 //
//                                                                        //
//  The second input is the pt spectrum of the daughter particle          //
// 								          //
//  Output would be the pt spectrum of the mother, based on the           //
//  correction factors computed from the Kinematics.root file             //
//                                                                        //
//                                                                        //  
//  Authors: Giuseppe E. Bruno           &  Fiorella Fionda               //
//           (Giuseppe.Bruno@ba.infn.it)    (Fiorella.Fionda@ba.infn.it)  //
////////////////////////////////////////////////////////////////////////////
 
#include "TH1F.h"
#include "TNtuple.h"
#include "TFile.h"
#include "TParticle.h"
#include "TArrayI.h"
#include "AliPtMothFromPtDaugh.h"
#include "AliStack.h"
#include "AliRunLoader.h"
#include "AliLog.h"

ClassImp(AliPtMothFromPtDaugh)
//_____________________________________________________________________________________
AliPtMothFromPtDaugh::AliPtMothFromPtDaugh():
  fDecayKine(0x0), 
  fWij(0x0),
  fFi(0x0),
  fWijMin(0x0),
  fFiMin(0x0),
  fHistoPtDaughter(0x0),
  fHistoPtMothers(0x0),
  fHistoPtMinMothers(0x0),
  fMothers(0x0),  
  fDaughter(0), 
  fReadKine(kFALSE),
  fyMothMax(0),
  fyMothMin(0),
  fyDaughMax(0),
  fyDaughMin(0),
  fUseEta(kFALSE),
  fAnalysisMode(kUserAnalysis)
   {
   //Default constructor
   }

//______________________________________________________________________________________
AliPtMothFromPtDaugh::AliPtMothFromPtDaugh(const AliPtMothFromPtDaugh& extraction) :
  TObject(extraction),
  fDecayKine(0),
  fWij(0),
  fFi(0),
  fWijMin(0),
  fFiMin(0),
  fHistoPtDaughter(0),
  fHistoPtMothers(0),
  fHistoPtMinMothers(0),
  fMothers(0),
  fDaughter(extraction.fDaughter),
  fReadKine(extraction.fReadKine),
  fyMothMax(extraction.fyMothMax),
  fyMothMin(extraction.fyMothMin),
  fyDaughMax(extraction.fyDaughMax),
  fyDaughMin(extraction.fyDaughMin),
  fUseEta(extraction.fUseEta),
  fAnalysisMode(extraction.fAnalysisMode)
  {
   // Copy constructor
   if(extraction.fHistoPtDaughter) fHistoPtDaughter =(TH1F*)extraction.fHistoPtDaughter->Clone("fHistoPtDaughter_copy");
   if(extraction.fHistoPtMothers) fHistoPtMothers = (TH1F*)extraction.fHistoPtMothers->Clone("fHistoPtMothers_copy");
   if(extraction.fHistoPtMinMothers) fHistoPtMinMothers = (TH1F*)extraction.fHistoPtMinMothers->Clone("fHistoPtMinMothers_copy");
  
   if(extraction.fWij){
     fWij = new Double_t*[2*(fHistoPtMothers->GetNbinsX())]; 
     for(int i=0;i<2*(fHistoPtMothers->GetNbinsX());i++) *(fWij+i)=new Double_t[fHistoPtDaughter->GetNbinsX()];
        for(int i=0;i<2*(fHistoPtMothers->GetNbinsX());i++){
          for(int j=0;j<fHistoPtDaughter->GetNbinsX();j++){
             fWij[i][j]=extraction.fWij[i][j];
             } 
           } 
     } 
   
   if(extraction.fFi){
     fFi=new Double_t[2*(fHistoPtMothers->GetNbinsX())];
     for(int i=0;i<2*(fHistoPtMothers->GetNbinsX());i++) fFi[i]=extraction.fFi[i];
     }
  
   if(extraction.fWijMin){ 
     fWijMin = new Double_t*[2*(fHistoPtMinMothers->GetNbinsX())];
     for(int i=0;i<2*(fHistoPtMinMothers->GetNbinsX());i++) *(fWijMin+i)=new Double_t[fHistoPtDaughter->GetNbinsX()];
        for(int i=0;i<2*(fHistoPtMinMothers->GetNbinsX());i++){
          for(int j=0;j<fHistoPtDaughter->GetNbinsX();j++){
             fWijMin[i][j]=extraction.fWijMin[i][j];
             }
           }
     }

   if(extraction.fFiMin){
     fFiMin=new Double_t[2*(fHistoPtMinMothers->GetNbinsX())];
     for(int i=0;i<2*(fHistoPtMinMothers->GetNbinsX());i++) fFiMin[i]=extraction.fFiMin[i];
     }

   if(extraction.fDecayKine) fDecayKine = (TNtuple*)extraction.fDecayKine->CloneTree(); 

   if(extraction.fMothers) fMothers = new TArrayI(*extraction.fMothers);
   
   extraction.Copy(*this);
  }

//______________________________________________________________________________________
AliPtMothFromPtDaugh& AliPtMothFromPtDaugh::operator=(const AliPtMothFromPtDaugh &extraction)
  { 
    // operator assignment
    if (this!=&extraction) { 
    this->~AliPtMothFromPtDaugh();
    new(this) AliPtMothFromPtDaugh(extraction); 
    }
    return *this; 
  }

//______________________________________________________________________________________
AliPtMothFromPtDaugh::~AliPtMothFromPtDaugh()
  {
   //Standard destructor
   if(fDecayKine) {delete fDecayKine;}
   fDecayKine=0;
   if(fMothers) {delete fMothers;}
   fMothers=0;
   if(fHistoPtMothers) { delete fHistoPtMothers; }
   fHistoPtMothers=0;
   if(fHistoPtMinMothers) { delete fHistoPtMinMothers;}
   fHistoPtMinMothers=0;
   if(fHistoPtDaughter) { delete fHistoPtDaughter; fHistoPtDaughter=0;}
   DeleteWeights(); 
   return;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::CreateWeights()
  {
   // Set mothers and daughters pdg codes if not
   // Put a control if mothers histograms binning-size, rapidity (or pseudoapidity) range
   // are setting. Read daughter histogram (histName) from the file (pathFileName)
   // Initialize dimensions for correction factors

   DeleteWeights();
   if(!fMothers)  {AliWarning("Set pdg codes of mothers by SetPdgMothers!\n"); return kFALSE;}
   if(!fDaughter) {AliWarning("Set pdg code of daughter by SetPdgDaughter!\n"); return kFALSE;}  
   if(!fHistoPtDaughter) { AliWarning("Daughter histogram doesn't exist! \n"); return kFALSE;} 
   
   //Set Rapidity or Pseudorapidity range for mothers if not
   if(!fyMothMax || !fyMothMin ){ AliWarning("Set rapidity range or pseudoRapidity range for mothers: use SetYmothers(ymin,ymax) or SetEtaMothers(etamin,etamax)"); return kFALSE;}   
   if(!fyDaughMax || !fyDaughMin){ AliWarning("Set rapidity range or pseudoRapidity range for daughters:use SetYdaughters(ymin,ymax) or SetEtaDaughters(etamin,etamax)"); return kFALSE;}  
   if(!fHistoPtMothers) {AliWarning("Call method SetBinsPtMoth to set pT-histogram "); return kFALSE;}
   if(!fHistoPtMinMothers){AliWarning("Call method SetBinsPtMinMoth to set pTmin-histogram "); return kFALSE;}

   Int_t nbinsM=(fHistoPtMothers->GetNbinsX()+2);
   Int_t nbinsD=(fHistoPtDaughter->GetNbinsX()+2);
   Int_t nbinsM_min=(fHistoPtMinMothers->GetNbinsX()+2);

   //Create pointers for weights to reconstruct daughter and mothers pT-spectra 
   fWij=new Double_t*[2*nbinsM];
   for(int i=0;i<2*nbinsM;i++) 
      {*(fWij+i)=new Double_t[nbinsD];}   
   fFi=new Double_t[2*nbinsM];   
    
   fWijMin=new Double_t*[2*nbinsM_min];
   for(int i=0;i<2*nbinsM_min;i++) 
      {*(fWijMin+i)=new Double_t[nbinsD];}
   fFiMin=new Double_t[2*nbinsM_min];
   AliInfo(Form("Pt-mothers distribution: pt_min = %f  pt_max=%f  n_bins=%d \n",
           fHistoPtMothers->GetBinLowEdge(1),fHistoPtMothers->GetBinLowEdge(nbinsM-1),nbinsM-2));
   AliInfo(Form("PtMinimum-mothers distribution: pt_min = %f  pt_max=%f  n_bins=%d \n",
           fHistoPtMinMothers->GetBinLowEdge(1),fHistoPtMinMothers->GetBinLowEdge(nbinsM_min-1),nbinsM_min-2));
   AliInfo(Form("Pt-daughters distribution: pt_min = %f  pt_max=%f n_bins=%d \n",
           fHistoPtDaughter->GetBinLowEdge(1),fHistoPtDaughter->GetBinLowEdge(nbinsD-1),nbinsD-2));
   return kTRUE;
  }  

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::ReadHistoPtDaught(TH1F *hist)
  {
   //Initialize daughter histograms with hist
   if(!hist) {AliWarning("Set correct histograms of daughter! It doesn't exist!\n"); return kFALSE;}
   if(fHistoPtDaughter) delete fHistoPtDaughter;
   fHistoPtDaughter = (TH1F*)hist->Clone();
   return kTRUE;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::DeleteWeights()
  {
   //delete correction factors   
   //delete histogram of daughters
   if(!fHistoPtMothers || !fHistoPtMinMothers) {AliWarning("Mothers histograms don't exist! Cannot delete correction factors"); return;}
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;  
   if(fWij){
     for(Int_t i=0; i<(2*nbinsM); i++) delete fWij[i];
     delete [] fWij; fWij=0;
    }
   if(fFi) { delete fFi; fFi=0;} 
   if(fWijMin){
     for(Int_t i=0; i<(2*nbinsM_min); i++) delete fWijMin[i];
     delete [] fWijMin; fWijMin=0;
    }
   if(fFiMin) { delete fFiMin; fFiMin=0;}
 
   return;
  }

//______________________________________________________________________________________
Double_t* AliPtMothFromPtDaugh::SetBinsSize(Double_t ptmin, Double_t ptmax,Int_t nbins,Double_t alpha)
  {
   // return a pointer of double which contains the binning size for (mothers) histograms:
   // alpha have to be > 0 and:  
   // alpha = 1 equal binning size
   // alpha < 1 increasing  " 
   // alpha > 1 decreasing  "
   if(ptmin<0 || ptmax<0 || nbins<=0 || alpha<=0) {AliWarning("Set correct bin-size: ptmin>=0, ptmax>=0, nbins>0, alpha>0! \n"); return 0;}
   Double_t *edgebin = new Double_t[nbins+1];
   Double_t ptmin1=TMath::Power(ptmin,alpha);
   Double_t ptmax1=TMath::Power(ptmax,alpha);
   Double_t size=(ptmax1-ptmin1)/nbins;
   for(int i=0;i<nbins+1;i++) *(edgebin+i)=TMath::Power((ptmin1+i*size),(Double_t)1/alpha);
   return edgebin;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetBinsPtMoth(Double_t ptmin, Double_t ptmax,Int_t nbins,Double_t alpha)
  {
   // Set bin size for pt-spectrum of mothers using SetBinsSize:
   // alpha have to be > 0 and:
   // alpha = 1 equal binning size
   // alpha < 1 increasing  " 
   // alpha > 1 decreasing  "
   Double_t* edges = SetBinsSize(ptmin,ptmax,nbins,alpha);
   SetBinsPtMoth(nbins,edges);
   delete edges;
   return;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetBinsPtMoth(Int_t nbins,Double_t *edgeBins)
  {
   //set bin size given by the pointer edgeBins for pt-spectrum of mothers:
   //the dimension of the pointer edgeBins is nbins+1 and the points
   //has to be written in increasing order 
   if(nbins<0) {AliWarning("Numbers of bins should be > 0 !\n"); return;}
   if(fHistoPtMothers) {delete fHistoPtMothers; fHistoPtMothers=0;}
   fHistoPtMothers=new TH1F("fHistoPtMothers","Reconstructed p_{T}(Mothers)-spectrum",nbins,edgeBins);   
   return;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetBinsPtMinMoth(Int_t nbins,Double_t *edgeBins)
  {
   //set bin size given by the pointer edgeBins for ptMin-spectrum of mothers:
   //the dimension of the pointer edgeBins is nbins+1 and the points
   //has to be written in increasing order 
   if(nbins<0) {AliWarning("Numbers of bins should be > 0 !\n"); return;}
   if(fHistoPtMinMothers) {delete fHistoPtMinMothers; fHistoPtMinMothers=0;}
   fHistoPtMinMothers = new TH1F("fHistoPtMinMothers","Reconstructed p_{T}^{MIN}(Mothers)-spectrum",nbins,edgeBins);
   return;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetBinsPtMinMoth(Double_t ptmin, Double_t ptmax,Int_t nbins,Double_t alpha)
  {
   // Set bin size for ptMin-spectrum of mothers using SetBinsSize:
   // alpha have to be > 0 and:
   // alpha = 1 equal binning size
   // alpha < 1 increasing  " 
   // alpha > 1 decreasing  "
   Double_t* edges = SetBinsSize(ptmin,ptmax,nbins,alpha);
   SetBinsPtMinMoth(nbins,edges);
   delete edges;
   return;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetPdgMothersPrivate(Int_t n_mothers,Int_t *pdgM)
  {
   // Set pdg codes of mothers given by the pointer pdgM for the analysis. 
   // This is a private method. 
   if(fMothers) { delete fMothers; fMothers = 0; }
   fMothers = new TArrayI(n_mothers,pdgM); 
   return;
  }
//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetPdgMothers(Int_t n_mothers,Int_t *pdgM)
  {
   // Set user pdg codes of mothers: first check 
   // that the kUserAnalysis is the selected Analysis_mode. 
   // If not print out a message of error. 
   if(fAnalysisMode!=kUserAnalysis) {
     AliWarning("Nothing done: set the mode to  kUserAnalysis first");
     return;
   }
   //Set pdg codes of mothers given by the pointer pdgM for the analysis 
   SetPdgMothersPrivate(n_mothers,pdgM);
   return;
  }
//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetBeautyMothers()
  {
   // Set pdg codes of beauty particles:
   // B-mesons (1-24) B-barions (25-59)
   Int_t PdgBeauty[]={511,521,513,523,10511,10521,10513,10523,20513,
      20523,515,525,531,10531,533,10533,205433,535,541,10541,543,10543,
      20543,545,5122,5112,5212,5222,5114,5214,5224,5132,5232,5312,5322,
      5314,5324,5332,5334,5142,5242,5412,5422,5414,5424,5342,5432,5434,
      5442,5444,5512,5522,5514,5524,5532,5534,5542,5544,5554};
   Int_t *pdgB=new Int_t[59];
   for(Int_t i=0;i<59;i++) pdgB[i]=PdgBeauty[i];
   SetPdgMothersPrivate(59,pdgB);
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::InitDefaultAnalysis()
 {
  // Set mothers and daughter pdg codes depending from the selected analysis. 
  // case kUserAnalysis: mothers and daughter are set by user (nothing to be done)
  // case kBtoJPSI: inclusive B-> J/Psi + X channels 
  // case kBtoEle: inclusive B-> e + X channels
  // case kBtoMuon: inclusive B-> mu + X channels
  // case kBtoD0: inclusive B-> D0 + X channels 
  
  switch(fAnalysisMode)
   {
   case kUserAnalysis:
    break; 
   case kBtoJPSI:
    SetBeautyMothers();
    fDaughter = 443;
    break;
   case kBtoEle: 
   SetBeautyMothers();
    fDaughter = 11;
    break;
   case kBtoMuon: 
    SetBeautyMothers();
    fDaughter = 13;
    break;
   case kBtoD0: 
    SetBeautyMothers();
    fDaughter = 421;
    break;
   }
  }

//______________________________________________________________________________________
Double_t* AliPtMothFromPtDaugh::GetBinsSize(const TH1F *hist,Int_t &n) const
  {
   // return the binning size of the histogram hist
   // n return the number of bins 
   Double_t* edges = (Double_t*)hist->GetXaxis()->GetXbins()->GetArray();
   n=hist->GetNbinsX();
   return edges;  
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::GetEtaMothers(Double_t &etaMin, Double_t &etaMax) const
  {
   // method to get the bounds of the pseudorapidity range 
   // for mothers. Return kTRUE if pseudorapidity is used and put 
   // pseudorapidity edges in etaMin and etaMax   
 
  if(fUseEta == kFALSE){
        AliWarning("You are using RAPIDITY range! \n"); 
        etaMin = 0.;
        etaMax = 0.;
        return kFALSE;
        }  
   etaMin = fyMothMin;
   etaMax = fyMothMax;  
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::GetEtaDaughter(Double_t &etaMin, Double_t &etaMax) const
  {
   // method to get the bounds of the pseudorapidity range 
   // for daughters. Return kTRUE if pseudorapidity is used and put 
   // pseudorapidity edges in etaMin and etaMax   

   if(fUseEta == kFALSE){
        AliWarning("You are using RAPIDITY range! \n"); 
        etaMin = 0.;
        etaMax = 0.; 
        return kFALSE;
        }
   etaMin = fyDaughMin;
   etaMax = fyDaughMax;
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::GetYMothers(Double_t &yMin, Double_t &yMax) const
  {
   // method to get the bounds of the rapidity range 
   // for mothers. Return kTRUE if rapidity is used and put 
   // rapidity edges in yMin and yMax   

   if(fUseEta == kTRUE){
        AliWarning("You are using PSEUDORAPIDITY range! \n"); 
        yMin = 0.;
        yMax = 0.;
        return kFALSE;
        }
   yMin = fyMothMin;
   yMax = fyMothMax;
   return kTRUE;
  }
 
//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::GetYDaughter(Double_t &yMin, Double_t &yMax) const
  {
   // method to get the bounds of the rapidity range 
   // for daughters. Return kTRUE if rapidity is used and put 
   // rapidity edges in yMin and yMax 

   if(fUseEta == kTRUE){
        AliWarning("You are using PSEUDORAPIDITY range! \n"); 
        yMin = 0.;
        yMax = 0.;
        return kFALSE;
        }
   yMin = fyDaughMin;
   yMax = fyDaughMax;
   return kTRUE;
  }

//______________________________________________________________________________________
Int_t* AliPtMothFromPtDaugh::GetPdgMothers(Int_t &n_mothers) const
  {
   // return the pointer to the array of pdg codes of mothers particles
   // if it exist. Put its dimension in n_mothers   
 
   if(!fMothers) {AliWarning("Mothers pdg are not defined! \n"); return 0x0;} 
   n_mothers = fMothers->GetSize(); 
   return fMothers->GetArray();
  }

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetW(Int_t i,Int_t j) const
  {
   // Return value of correction factors Wij at the position i (pt-mothers bin index)-
   // j (pt-daughter bin index). Bin 0 is the underflow, bin nbins+1 the overflow. 
   // If Wij don't exist or the indices i or j are out of the variability range return 0 
    
   if(!fWij) {AliWarning("Correction factors Wij are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMothers) {AliWarning("mothers pt-histogram doesn't exist!\n"); return 0;}
   if(!fHistoPtDaughter) {AliWarning("daughters pt-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   if(i<0 || i>nbinsM-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow ",0,nbinsM-1,nbinsM-1)); return 0;} 
  
   if(j<0 || j>nbinsD-1) {AliWarning(Form("Index j out of range: %d =< j =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsD-1,nbinsD-1)); return 0;}  
   return fWij[i][j];
  } 

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetStatErrW(Int_t i,Int_t j) const
  {
   // Return value of statistical error on correction factors Wij at the position 
   // i (pt-mothers bin index)- j (pt-daughter bin index). Bin 0 is the underflow, 
   // bin nbins+1 the overflow. If Wij don't exist or the indices i or j are out of the 
   // variability range return 0 
  
   if(!fHistoPtMothers) {AliWarning("mothers pt-histogram doesn't exist!\n"); return 0;}
   if(!fHistoPtDaughter) {AliWarning("daughters pt-histogram doesn't exist!\n"); return 0;}
 
   if(!fWij) {AliWarning("Correction factors Wij are not been evaluated yet!\n"); return 0;}
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   if(i<0 || i>nbinsM-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM-1,nbinsM-1)); return 0;}
   if(j<0 || j>nbinsD-1) {AliWarning(Form("Index j out of range: %d =< j =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsD-1,nbinsD-1)); return 0;}
   return fWij[i+nbinsM][j];
  }

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetF(Int_t i) const
  {
   // Return value of correction factors Fi at the position i (pt-mothers bin index).
   // Bin 0 is the underflow, bin nbins+1 the overflow. 
   // If Fi don't exist or the index i is out of the variability range return 0     
  
   if(!fFi) {AliWarning("Correction factors Fi are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMothers) {AliWarning("mothers pt-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   if(i<0 || i>nbinsM-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM-1,nbinsM-1)); return 0;}
   return fFi[i];
  }

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetStatErrF(Int_t i) const
  {
   // Return statistical error on correction factors Fi at the position i (pt-mothers bin index).
   // Bin 0 is the underflow, bin nbins+1 the overflow. 
   // If Fi don't exist or the index i is out of the variability range return 0     

   if(!fFi) {AliWarning("Correction factors Fi are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMothers) {AliWarning("mothers pt-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   if(i<0 || i>nbinsM-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM-1,nbinsM-1)); return 0;}

   return fFi[i+nbinsM];
  }
 
//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetWmin(Int_t i,Int_t j) const
  {
   // Return value of correction factors Wij_min at the position i (ptMin-mothers bin index)-
   // j (pt-daughter bin index). Bin 0 is the underflow, bin nbins+1 the overflow. 
   // If Wij_min don't exist or the indices i or j are out of the variability range return 0  
    
   if(!fWijMin) {AliWarning("Correction factors Wij_min are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMinMothers) {AliWarning("mothers ptMin-histogram doesn't exist!\n"); return 0;}
   if(!fHistoPtDaughter) {AliWarning("daughters pt-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   if(i<0 || i>nbinsM_min-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM_min-1,nbinsM_min-1)); return 0;}
   if(j<0 || j>nbinsD-1) {AliWarning(Form("Index j out of range: %d =< j =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsD-1,nbinsD-1)); return 0;}
   return fWijMin[i][j];
  }

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetStatErrWmin(Int_t i,Int_t j) const
  {
   // Return value of statistical error on correction factors Wij_min at the position 
   // i (ptMin-mothers bin index)- j (pt-daughter bin index). Bin 0 is the underflow, 
   // bin nbins+1 the overflow. If Wij_min don't exist or the indices i or j are out of the 
   // variability range return 0 

   if(!fWijMin) {AliWarning("Correction factors Wij_min are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMinMothers) {AliWarning("mothers ptMin-histogram doesn't exist!\n"); return 0;}
   if(!fHistoPtDaughter) {AliWarning("daughters pt-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   if(i<0 || i>nbinsM_min-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM_min-1,nbinsM_min-1)); return 0;}
   if(j<0 || j>nbinsD-1) {AliWarning(Form("Index j out of range: %d =< j =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsD-1,nbinsD-1)); return 0;}
 
   return fWijMin[i+nbinsM_min][j];
  }

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetFmin(Int_t i) const
  {
   // Return value of correction factors Fi_min at the position i (ptMin-mothers bin index).
   // Bin 0 is the underflow, bin nbins+1 the overflow. 
   // If Fi_min don't exist or the index i is out of the variability range return 0     

   if(!fFiMin) {AliWarning("Correction factors Fi_min are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMinMothers) {AliWarning("mothers ptMin-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM_min=fHistoPtMothers->GetNbinsX()+2;
   if(i<0 || i>nbinsM_min-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM_min-1,nbinsM_min-1)); return 0;}
   return fFiMin[i];
  }

//______________________________________________________________________________________
Double_t AliPtMothFromPtDaugh::GetStatErrFmin(Int_t i) const
  {
   // Return statistical error on correction factors Fi_min at the position i (ptMin-mothers bin index).
   // Bin 0 is the underflow, bin nbins+1 the overflow. 
   // If Fi_min don't exist or the index i is out of the variability range return 0     

   if(!fFiMin) {AliWarning("Correction factors Fi_min are not been evaluated yet!\n"); return 0;}
   if(!fHistoPtMinMothers) {AliWarning("mothers ptMin-histogram doesn't exist!\n"); return 0;}
   Int_t nbinsM_min=fHistoPtMothers->GetNbinsX()+2;
   if(i<0 || i>nbinsM_min-1) {AliWarning(Form("Index i out of range: %d =< i =< %d. Bin 0 is the underflow - bin %d is the overflow.",0,nbinsM_min-1,nbinsM_min-1)); return 0;}

   return fFiMin[i+nbinsM_min];
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::ReadKinematics(const char *pathFileName)
  {
   /* Read the kinematic to create the Ntupla and store it in the file DecayKine.root:
      if it is already created read it from the file DecayKine.root. Input: path of the
      file galice.root. 
      TNtupla contains:  pdg of Mothers  
                          px      "       
                          py	  " 
                          pz      "
                        rapidity  "
                         eta      "
                          pdg of Daughter
                          px      "       
                          py      " 
                          pz      "
                        rapidity  "
                         eta      "                                                  */ 
    if(fReadKine)   
     {
      TFile *f = new TFile("DecayKine.root","READ");
      if(!f) {AliWarning("File DecayKine.root with TNtupla doesn't exist!"); return kFALSE;}
      fDecayKine=(TNtuple*)gDirectory->Get("DecayKine");
      if(!fDecayKine) { AliWarning("The TNtupla doesn't exist!\n"); return kFALSE;}
      return kTRUE;
     }
   if(!fMothers) { AliWarning("Set Pdg codes for mothers before to call ReadKinematic!\n"); return kFALSE;}
   if(!fDaughter) { AliWarning("Set Pdg codes for daughter before to call ReadKinematic!\n"); return kFALSE;}
   //declare the out file in which is stored the Ntupla
   TFile *out = new TFile("DecayKine.root","RECREATE");
   //create the TNtupla
   if(fDecayKine) {delete fDecayKine; fDecayKine = 0;}
   fDecayKine=new TNtuple("DecayKine","Decay kinematics",
   "pdgM:pxM:pyM:pzM:yM:etaM:pdgD:pxD:pyD:pzD:yD:etaD");   
   AliRunLoader *RunLoader = AliRunLoader::Open(pathFileName);
   if(!RunLoader) { AliWarning("File galice.root not found!\n"); return kFALSE;}
   AliStack *stack=NULL;  
   RunLoader->LoadHeader();
   RunLoader->LoadKinematics();
   float *inf = new float[12]; 
   Int_t pdg;
   Int_t nEvents = RunLoader->GetNumberOfEvents();
   //event loop
   for(Int_t iEvent = 0; iEvent < nEvents; ++iEvent){
    RunLoader->GetEvent(iEvent);
    stack = RunLoader->Stack();
    Int_t nPrims = stack->GetNprimary();
    for (Int_t iTrack = 0; iTrack < nPrims; ++iTrack) {
        TParticle *part = stack->Particle(iTrack);
        pdg=TMath::Abs(part->GetPdgCode());
        Double_t y,y2;
           //check if particle is in mothers list  
           Int_t labelDaugh; 
           if(IsMothers(pdg))
           {
           //check if mother particle has a selected daugh
           if(!IsSelectedDaugh(part,labelDaugh,stack)) continue;
           TParticle *pDaugh=stack->Particle(labelDaugh);
           Rapidity(part,y);
           Rapidity(pDaugh,y2);

           inf[0]=part->GetPdgCode();
           inf[1]=part->Px();
           inf[2]=part->Py();
           inf[3]=part->Pz();
           inf[4]=y;
           inf[5]=part->Eta();
           inf[6]=pDaugh->GetPdgCode();
           inf[7]=pDaugh->Px();
           inf[8]=pDaugh->Py();
           inf[9]=pDaugh->Pz();
           inf[10]=y2;
           inf[11]=pDaugh->Eta();          
  
           fDecayKine->Fill(inf);
           }//close if statement for mothers particles
        }//end of tracks loop
     }//end of events loop

     RunLoader->UnloadHeader();
     RunLoader->UnloadKinematics();
     RunLoader->UnloadTrigger();
     RunLoader->UnloadTrackRefs();
     delete RunLoader;
     out->cd();   
     fDecayKine->Write();
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::IsMothers(Int_t pdgCode)
  {
   //return kTRUE if pdgCode is in the list of pdg codes of mothers
   Int_t dim = fMothers->GetSize();
   for(int i=0;i<dim;i++) { if(pdgCode==fMothers->GetAt(i)) return kTRUE; }
   return kFALSE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::IsSelectedDaugh(const TParticle *part, Int_t &labelDaugh, AliStack *stack)
  {
   // return kTRUE if particle part has the selected daughter
   // if yes put the label of the track in labelDaugh  
   TParticle *daugh;
   Int_t nDg=part->GetNDaughters();
   if(nDg<=0) {AliWarning("I have no daugh!\n"); return kFALSE;}
   for(int i=part->GetFirstDaughter();i<part->GetLastDaughter()+1;i++)
     {
      daugh=stack->Particle(i);
      if(TMath::Abs(daugh->GetPdgCode())==fDaughter) {
        labelDaugh=i;
        return kTRUE;
      }
     }
   return kFALSE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::Rapidity(const TParticle *particle, Double_t &y)
  {
   // Evaluated rapidity of particle  and put it in y. Return kFALSE if
   // cannot compute rapidity 
   y=-999;
   if(particle->Energy()-particle->Pz()<=0) return kFALSE;
   y=0.5*TMath::Log((particle->Energy()+particle->Pz())/(particle->Energy()-particle->Pz()));
   return kTRUE;
  }

//______________________________________________________________________________________
Int_t AliPtMothFromPtDaugh::GiveBinIndex(Double_t Ptpart,const TH1F *ptHist)
  {
   // Return the bin index of pt respect to the binning size of ptHist
   // bin 0 is the underflow - nbins+1 is the overflow  
   Int_t nbins=ptHist->GetNbinsX();
   Int_t index=0;
   for(int i=1;i<nbins+2;i++)
     {
      if(Ptpart<(ptHist->GetBinLowEdge(i)))
      {
      index=i-1;  
      break;
      }
     }
   return index;
  }

//______________________________________________________________________________________
Bool_t  AliPtMothFromPtDaugh::CutDaugh(Double_t yD,Double_t ptD)
  {
   // put a control for rapidity yD and transverse momentum ptD of daughter
   // return kTRUE if  fyDaughMin < yD < fyDaughMax and ptMinDaugh < ptD < ptMaxDaugh   
   Double_t ptMinDaugh = fHistoPtDaughter->GetBinLowEdge(1);
   Double_t ptMaxDaugh = fHistoPtDaughter->GetBinLowEdge(fHistoPtDaughter->GetNbinsX());
   if( yD < fyDaughMin || yD > fyDaughMax ) return kFALSE;
   if( ptD > ptMaxDaugh || ptD < ptMinDaugh ) return kFALSE;
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::EvaluateWij()
  {
   // Evaluate correction factors using to extract the ptRaw and
   // ptMinRaw distributions. Statistical errors on those are computed too
  
   if(!fHistoPtMothers || !fHistoPtDaughter || !fHistoPtMinMothers) 
   {AliWarning("Control mother and daughter histograms!\n"); return kFALSE;}

   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Float_t pdgM, pdgD, pxM,pyM,pzM,yM,pxD,pyD,pzD,yD,etaM,etaD,cutVarM,cutVarD;
   Double_t* entries=new Double_t[nbinsD];
   for(int i=0;i<nbinsD;i++) {entries[i]=0.;}
   Int_t i,j,iMin;
   if(!fDecayKine) {AliWarning("TNtupla is not defined!\n"); return kFALSE;}
   fDecayKine->SetBranchAddress("pdgM",&pdgM);
   fDecayKine->SetBranchAddress("pxM",&pxM);
   fDecayKine->SetBranchAddress("pyM",&pyM);
   fDecayKine->SetBranchAddress("pzM",&pzM);
   fDecayKine->SetBranchAddress("yM",&yM);
   fDecayKine->SetBranchAddress("etaM",&etaM);
   fDecayKine->SetBranchAddress("pdgD",&pdgD);
   fDecayKine->SetBranchAddress("pxD",&pxD);
   fDecayKine->SetBranchAddress("pyD",&pyD);
   fDecayKine->SetBranchAddress("pzD",&pzD);
   fDecayKine->SetBranchAddress("yD",&yD);
   fDecayKine->SetBranchAddress("etaD",&etaD);
   Double_t ptD,ptM;
   // Initialize correction factors for pT and pTmin if those exist
   if(!fWij) {AliWarning("Correction factors Wij have not been created!\n"); return kFALSE;}
   if(!fWijMin) {AliWarning("Correction factors Wij_min have not been created!\n"); return kFALSE;}
   for(int i=0;i<(2*nbinsM);i++){
     for(int j=0;j<nbinsD;j++){
        fWij[i][j]=0;
        }
      }
   for(int i=0;i<(2*nbinsM_min);i++){
     for(int j=0;j<nbinsD;j++){
       fWijMin[i][j]=0;
      }
    }
   Int_t nentries = (Int_t)fDecayKine->GetEntries();
   Int_t fNcurrent=0;
   Int_t nb = (Int_t)fDecayKine->GetEvent(fNcurrent);
   for (Int_t iev=0; iev<nentries; iev++){
   // check if rapidity or pseudorapidity range is set
   if(fUseEta == kFALSE) {cutVarD = yD; cutVarM = yM;}
   else {cutVarD = etaD; cutVarM = etaM;}
   ptD=TMath::Sqrt(pxD*pxD+pyD*pyD);
   ptM=TMath::Sqrt(pxM*pxM+pyM*pyM);
   pdgM = TMath::Abs(pdgM);
   if((TMath::Abs(pdgD))==fDaughter && IsMothers((Int_t)pdgM))
      {
       j=GiveBinIndex(ptD,fHistoPtDaughter);
       i=GiveBinIndex(ptM,fHistoPtMothers);
       iMin=GiveBinIndex(ptM,fHistoPtMinMothers);
       if(!CutDaugh(cutVarD,ptD)) { fNcurrent++; nb = (Int_t)fDecayKine->GetEvent(fNcurrent); continue;}
         if(cutVarM>fyMothMin && cutVarM<fyMothMax){
         fWij[i][j]+=1.;
         for(int k=0;k<iMin+1;k++) {fWijMin[k][j]+=1.;}
          }
         entries[j]++;
         }
    fNcurrent++;
    nb = (Int_t)fDecayKine->GetEvent(fNcurrent);
   }
  for(int j=0;j<nbinsD;j++){
      for(int i=0;i<nbinsM;i++){
         if(entries[j]!=0){
           fWij[i][j]=fWij[i][j]/entries[j];
           // evaluate statistical errors on fWij
           fWij[i+nbinsM][j]=(TMath::Sqrt(fWij[i][j]*(1-(fWij[i][j]/entries[j]))))/entries[j];
           }
         else{
           // if there are no entries in the bin-j of daughter distribution 
           // set factor = -1 and error = 999
           *(*(fWij+i)+j)=-1;
           *(*(fWij+nbinsM+i)+j)=999;
           }
         }
      for(int i=0;i<nbinsM_min;i++){
         if(entries[j]!=0){
            fWijMin[i][j]=fWijMin[i][j]/entries[j];
            //evaluate statistical errors on fWijMin
            fWijMin[i+nbinsM_min][j] = (TMath::Sqrt(fWijMin[i][j]*(1-(fWijMin[i][j]/entries[j]))))/entries[j]; 
            }
             else{
            //if there are no entries set correction factor = -1 and error = -999
            fWijMin[i][j]=-1;
            fWijMin[i+nbinsM_min][j]=999;
           }
        }
    }
   delete entries;
   return kTRUE;
  }


//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::EvaluateFi()
  {  
   // Evaluate acceptance correction factors that are applied on the 
   // raw distributions. Statistical errors on those are computed too  

   if(!fHistoPtMothers || !fHistoPtMinMothers)
   {AliWarning("Control mother histograms!\n"); return kFALSE;}

   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Double_t* entries=new Double_t[nbinsM];
   Double_t* entries1=new Double_t[nbinsM_min];
   if(!fFi) {AliWarning("Correction factors Fi have not been created!\n"); return kFALSE;}
   if(!fFiMin) {AliWarning("Correction factors Fi_min have not been created!\n"); return kFALSE;}
   //initialize the correction factor for pT and pTmin
   for(int i=0;i<nbinsM;i++){
       fFi[i]=0.; //for correction factors
       fFi[i+nbinsM]=0.; //for statistical error on correction factors
       entries[i]=0.;
      }
   for(int i=0;i<nbinsM_min;i++){
       entries1[i]=0.;
       fFiMin[i]=0.; //for correction factors
       fFiMin[i+nbinsM_min]=0.; //for statistical error on correction factors       
      }
   Float_t pdgM, pdgD, pxM,pyM,pzM,yM,pxD,pyD,pzD,yD,etaM,etaD,cutVarD,cutVarM;
   Int_t i,iMin;
   if(!fDecayKine) {AliWarning("TNtupla is not defined!\n"); return kFALSE;} 
   fDecayKine->SetBranchAddress("pdgM",&pdgM);
   fDecayKine->SetBranchAddress("pxM",&pxM);
   fDecayKine->SetBranchAddress("pyM",&pyM);
   fDecayKine->SetBranchAddress("pzM",&pzM);
   fDecayKine->SetBranchAddress("yM",&yM);
   fDecayKine->SetBranchAddress("etaM",&etaM);
   fDecayKine->SetBranchAddress("pdgD",&pdgD);
   fDecayKine->SetBranchAddress("pxD",&pxD);
   fDecayKine->SetBranchAddress("pyD",&pyD);
   fDecayKine->SetBranchAddress("pzD",&pzD);
   fDecayKine->SetBranchAddress("yD",&yD);
   fDecayKine->SetBranchAddress("etaD",&etaD);
   Double_t ptD,ptM;

   Int_t nentries = (Int_t)fDecayKine->GetEntries();
   Int_t fNcurrent=0;
   Int_t nb = (Int_t)fDecayKine->GetEvent(fNcurrent);

   for (Int_t iev=0; iev<nentries; iev++){
    pdgM = TMath::Abs(pdgM);
    if((TMath::Abs(pdgD))==fDaughter && IsMothers((Int_t)pdgM))
     {
     //check if rapidity or pseudorapidity range is set
     if(fUseEta == kFALSE) {cutVarD = yD; cutVarM = yM;}
     else {cutVarD = etaD; cutVarM = etaM;}
     ptD=TMath::Sqrt(pxD*pxD+pyD*pyD);
     ptM=TMath::Sqrt(pxM*pxM+pyM*pyM);
     i=GiveBinIndex(ptM,fHistoPtMothers);
     iMin=GiveBinIndex(ptM,fHistoPtMinMothers);
        if(cutVarM<fyMothMin || cutVarM>fyMothMax){ 
        fNcurrent++; nb = (Int_t)fDecayKine->GetEvent(fNcurrent); continue;}
     entries[i]++;
     for(int k=0; k<iMin+1;k++) {entries1[k]+=1;}
     if(!CutDaugh(cutVarD,ptD)) {fNcurrent++; nb = (Int_t)fDecayKine->GetEvent(fNcurrent); continue;}
     fFi[i]+=1.;
     for(int k=0; k<iMin+1;k++) {fFiMin[k]+=1.;}
     }
    fNcurrent++;
    nb = (Int_t) fDecayKine->GetEvent(fNcurrent);
    }
    
  for(int i=0;i<nbinsM;i++){
       if(entries[i]!=0){
         fFi[i]/=entries[i];
         fFi[i+nbinsM]=(TMath::Sqrt(fFi[i]*(1-(fFi[i]/entries[i]))))/entries[i];
        }
      else{
       	 fFi[i]=-1;
	 fFi[i+nbinsM]=999;
	}
      }
  for(int i=0;i<nbinsM_min;i++){
     if(entries1[i]!=0){
	fFiMin[i]/=entries1[i];
	fFiMin[i+nbinsM_min]=(TMath::Sqrt(fFiMin[i]*(1-(fFiMin[i]/entries1[i]))))/entries1[i];
       }
      else {fFiMin[i]=-1; fFiMin[i+nbinsM_min]=999;}
     }
   delete entries;
   delete entries1;
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::EvaluatePtMothRaw(TH1F *histoPt, TH1F *histoPtMin)
  {
   //Apply the fWij and fWijMin on the daughter distribution
   //in order to evaluate the pt and ptMin raw distributions for mothers

   if(!fHistoPtMothers || !fHistoPtDaughter || !fHistoPtMinMothers)
   {AliWarning("Control mother and daughter histograms!\n"); return kFALSE;}

   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Double_t lowedge=0.;
   Double_t wfill=0.;
   Double_t wMinfill=0.;
   for(int j=0;j<nbinsD;j++){
     for(int i=0;i<nbinsM;i++){
         lowedge=fHistoPtMothers->GetBinCenter(i);
         if(fWij[i][j]>=0) wfill=fWij[i][j]*(fHistoPtDaughter->GetBinContent(j));
         histoPt->Fill(lowedge,wfill);
         }
     for(int i=0;i<nbinsM_min;i++){
         lowedge=fHistoPtMinMothers->GetBinLowEdge(i);
         if(fWijMin[i][j]>=0) wMinfill=fWijMin[i][j]*(fHistoPtDaughter->GetBinContent(j));
         histoPtMin->Fill(lowedge,wMinfill);
         }
      }
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::EvaluateErrPt(Double_t *erStat)
  {
   // Evaluate the statistical error on the pt-mothers distribution.
   // sigmaX: contribution that came from the measured distibution
   // sigmaWij: contribution that came from the fWij factors
   // sigmaFi: contribution that came from the fFi factors 

   if(!fHistoPtMothers || !fHistoPtDaughter)
   {AliWarning("Control mother(pt) and daughter histograms!\n"); return kFALSE;}
 
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   Double_t m=0;
   Double_t  sigmaX, sigmaWij, sigmaFi;
   for(int i=0;i<nbinsM;i++){
       sigmaX=0.;
       sigmaWij=0.;
       sigmaFi=0;
       for(int j=0;j<nbinsD;j++){
          if(fWij[i][j]>=0){
          sigmaX+=(((fWij[i][j])*(fWij[i][j]))*((fHistoPtDaughter->GetBinError(j))*(fHistoPtDaughter->GetBinError(j))));
          sigmaWij+=((fHistoPtDaughter->GetBinContent(j))*(fHistoPtDaughter->GetBinContent(j)))*(fWij[i+nbinsM][j]*fWij[i+nbinsM][j]);
          sigmaFi+=(fWij[i][j])*(fHistoPtDaughter->GetBinContent(j));
          }
       } 
      if(fFi[i]>0) sigmaFi=((sigmaFi*sigmaFi)*(fFi[i+nbinsM]*fFi[i+nbinsM]))/(fFi[i]*fFi[i]);
      m=TMath::Sqrt(sigmaX+sigmaWij+sigmaFi);
      if(fFi[i]>0) erStat[i]=(1/fFi[i])*m;
      else erStat[i]=999;  
    }
   return kTRUE;
  }

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::EvaluateErrPtMin(Double_t *erStat)
  {
   // Evaluate statistical error on ptMin mothers distribution
   // sigmaMinX: contribution that came from the measured distibution
   // sigmaMinWij: contribution that came from the fWijMin factors
   // sigmaMinFi: contribution that came from the fFiMin factors  
   
   if(!fHistoPtDaughter || !fHistoPtMinMothers)
   {AliWarning("Control mother(ptMin) and daughter histograms!\n"); return kFALSE;}

   Int_t nbinsD=fHistoPtDaughter->GetNbinsX()+2;
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Double_t m1=0;
   Double_t sigmaMinX;
   Double_t sigmaMinWij;
   Double_t sigmaMinFi;
      for(int i=0;i<nbinsM_min;i++){
        sigmaMinX=0.;
        sigmaMinWij=0.;
        sigmaMinFi=0.;
           for(int j=0;j<nbinsD;j++){
            if(fWijMin[i][j]>=0){
            sigmaMinX+=(((fWijMin[i][j])*(fWijMin[i][j]))*((fHistoPtDaughter->GetBinError(j))*(fHistoPtDaughter->GetBinError(j))));
            sigmaMinWij+=((fHistoPtDaughter->GetBinContent(j))*(fHistoPtDaughter->GetBinContent(j)))*(fWijMin[i+nbinsM_min][j]*fWijMin[i+nbinsM_min][j]);
            sigmaMinFi+=(fWijMin[i][j])*(fHistoPtDaughter->GetBinContent(j));
           }
         }
    if(fFiMin[i]>0) sigmaMinFi=((sigmaMinFi*sigmaMinFi)*(fFiMin[i+nbinsM_min]*fFiMin[i+nbinsM_min]))/(fFiMin[i]*fFiMin[i]);
    m1=TMath::Sqrt(sigmaMinX+sigmaMinWij+sigmaMinFi);
    if(fFiMin[i]>0) erStat[i]=(1/fFiMin[i])*m1;
    else erStat[i]=999;
    }

   return kTRUE;
  } 

//______________________________________________________________________________________
Bool_t AliPtMothFromPtDaugh::EvaluatePtMoth()
  {
   // Evaluate pt and ptMin distribution for mothers
   // First evaluate the sigma raw distribution by calling EvaluatePtMothRaw
   // then evaluate the pt and ptMin mothers distribution.  
   // Statistical errors on those distributions are evaluated too.  

   if(!EvaluateWij()) return kFALSE;
   if(!EvaluateFi())  return kFALSE;

   // reset pt and ptMin mothers histograms
   fHistoPtMothers->Reset();
   fHistoPtMinMothers->Reset();

   TH1F *histoPt=(TH1F*)fHistoPtMothers->Clone();
   TH1F *histoPtMin=(TH1F*)fHistoPtMinMothers->Clone();
   EvaluatePtMothRaw(histoPt,histoPtMin);
   Int_t nbinsM=fHistoPtMothers->GetNbinsX()+2;
   Int_t nbinsM_min=fHistoPtMinMothers->GetNbinsX()+2;
   Double_t *erPtStat=new Double_t[nbinsM+2];
   EvaluateErrPt(erPtStat);
   Double_t *erPtMinStat=new Double_t[nbinsM_min+2];
   EvaluateErrPtMin(erPtMinStat); 
   Double_t lowedge=0;
   Double_t fwfill;
   Double_t fMinfill;
   for(int i=0;i<nbinsM;i++){
      fwfill=0.;
      lowedge=fHistoPtMothers->GetBinCenter(i);
       if(fFi[i]>0){
        fwfill=(histoPt->GetBinContent(i))/fFi[i];
        fHistoPtMothers->Fill(lowedge,fwfill);
        fHistoPtMothers->SetBinError(i,erPtStat[i]);
       }
      }
   for(int i=0;i<nbinsM_min;i++){
      fMinfill=0.;
      lowedge=fHistoPtMinMothers->GetBinCenter(i);
      if(fFiMin[i]>0){
         fMinfill=(histoPtMin->GetBinContent(i))/fFiMin[i];
         fHistoPtMinMothers->Fill(lowedge,fMinfill);
         fHistoPtMinMothers->SetBinError(i,erPtMinStat[i]);
        }
      }
   return kTRUE;
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::WritePtMothHistoToFile(char *fileOutName)
  {
   // Write pt and ptMin histograms of mothers in a file 
   // with name fileOutName. Default name is "Mothers.root".
   AliWarning(Form("Write mothers histograms in the file %s \n",fileOutName));
   if(!fHistoPtMothers) {AliWarning("Cannot write pt-mothers histogram! It doesn't exists!"); return;}
   if(!fHistoPtMinMothers)  { AliWarning("Cannot write ptMin-mothers histogram! It doesn't exists!"); return;} 
   TFile *outFile = TFile::Open(fileOutName,"RECREATE");
   outFile->cd();
   fHistoPtMothers->Write();
   fHistoPtMinMothers->Write();
   outFile->Close();
   return;  
  }

//______________________________________________________________________________________
void AliPtMothFromPtDaugh::SetPdgDaugh(Int_t pdgD)
  { 
   // Set pdg code for daughter particle. Check 
   // that the kUserAnalysis is the selected Analysis_mode. 
   // If not print out a message of error. 
   switch(fAnalysisMode)
    {
    case kUserAnalysis:
     fDaughter = pdgD;
     break;
    case kBtoJPSI:
     if(pdgD!= 443) {AliWarning("Nothing done: first change AnalysisMode to kUserAnalysis");}
     else {fDaughter = pdgD;} 
     break;
    case kBtoEle: 
     if(pdgD!= 11) {AliWarning("Nothing done: first change AnalysisMode to kUserAnalysis");}
     else {fDaughter = pdgD;} 
     break;
    case kBtoMuon: 
     if(pdgD!= 13) {AliWarning("Nothing done: first change AnalysisMode to kUserAnalysis");}
     else {fDaughter = pdgD;} 
     break; 
    case kBtoD0: 
     if(pdgD!= 421) {AliWarning("Nothing done: first change AnalysisMode to kUserAnalysis");}
     else {fDaughter = pdgD;}
     break;
    }
   return;
  }
