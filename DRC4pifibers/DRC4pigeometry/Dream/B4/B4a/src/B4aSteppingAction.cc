//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
// $Id: B4aSteppingAction.cc 68058 2013-03-13 14:47:43Z gcosmo $
// 
/// \file B4aSteppingAction.cc
/// \brief Implementation of the B4aSteppingAction class

#include "B4aSteppingAction.hh"
#include "B4aEventAction.hh"
#include "B4DetectorConstruction.hh"
#include "G4Material.hh"
#include "G4UnitsTable.hh"

#include "G4Step.hh"
#include "G4RunManager.hh"
#include <stdlib.h>

#include "G4OpBoundaryProcess.hh"
//#include "Fiber_Info.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B4aSteppingAction::B4aSteppingAction(
                      const B4DetectorConstruction* detectorConstruction,
                      B4aEventAction* eventAction)
  : G4UserSteppingAction(),
    fDetConstruction(detectorConstruction),
    fEventAction(eventAction)
{
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

B4aSteppingAction::~B4aSteppingAction()
{ 
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void B4aSteppingAction::UserSteppingAction(const G4Step* step)
{
	
  // get volume of the current pre-step
  G4VPhysicalVolume* PreStepVolume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
  G4double energydeposited = step->GetTotalEnergyDeposit();
  G4double steplength = step->GetStepLength();
  
  //define Birk's constant
  double k_B = 0.126; 
  G4double saturatedenergydeposited = 0.;
 
  if (PreStepVolume->GetName() != "World"){
  	fEventAction->Addenergy(energydeposited);
  	if (PreStepVolume->GetLogicalVolume()->GetMaterial()->GetName() == "Copper"){
  		G4double copynumbertower = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(); 
    	G4double copynumberslice = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(1); 
    	if (copynumbertower > 0){ //im in barrel right or endcap right
     		fEventAction->AddVectorR(energydeposited,copynumbertower, copynumberslice);
    	}
  		if (copynumbertower < 0){ //im in barrel left or endcap left
  	 		fEventAction->AddVectorL(energydeposited, copynumbertower, copynumberslice);
        }
    }
  }

  if (PreStepVolume->GetName() != "World" ) {
    if (step->GetTrack()->GetDefinition()->GetParticleName() == "e-" || step->GetTrack()->GetDefinition()->GetParticleName() == "e+"){
      //Function to add up energy deposited by em component
      fEventAction->Addem(energydeposited);
    }
  }

  if ( step->GetTrack()->GetTrackID() == 1 && step->GetTrack()->GetCurrentStepNumber() == 1){
    // Function to save primary particle energy and name
    fEventAction->SavePrimaryParticle(step->GetTrack()->GetDefinition()->GetParticleName());
    fEventAction->SavePrimaryEnergy(step->GetTrack()->GetKineticEnergy());
  }
	////////////////////////////////////////////////////////////////////
	//DECOMMENT if you want the tau decays products position information
	
	if(step->GetTrack()->GetParentID() == 1){
		fEventAction->WriteTracking_Info(step->GetTrack()->GetTrackID(),step->GetPreStepPoint()->GetPosition(),step->GetTrack()->GetDefinition()->GetParticleName(),step->GetTrack()->GetKineticEnergy());
	}
	/*
	if(((step->GetTrack()->GetParentID() == 4 || step->GetTrack()->GetParentID() == 3) && (step->GetTrack()->GetDefinition()->GetParticleName()=="gamma"))|| (step->GetTrack()->GetParentID() == 2 && step->GetTrack()->GetDefinition()->GetParticleName()=="mu-")){
		fEventAction->WriteTracking_Info(step->GetTrack()->GetTrackID(),step->GetPreStepPoint()->GetPosition(),step->GetTrack()->GetDefinition()->GetParticleName(),step->GetTrack()->GetKineticEnergy());
	}*/
	////////////////////////////////////////////////////////////////////
		
  //Here I compute and save all informations about scintillating and Cherenkov fibers
  std::string Fiber;
  std::string S_fiber = "fiberCoreScint";
  std::string C_fiber = "fiberCoreChere";
  Fiber = PreStepVolume->GetName(); //name of current step fiber
  
  if ( strstr(Fiber.c_str(),S_fiber.c_str())){ //it's a scintillating fiber
    //Function to add up energy depoisted in scintillating fibers:
    //- as signal saturated by Birk's law in VectorSignals
    //- as regular energy deposition in all scintillating fibers in EnergyScin

    if(step->GetTrack()->GetDefinition()->GetPDGCharge() != 0.){
        if (steplength != 0)
                {
                    saturatedenergydeposited = (energydeposited/steplength) / ( 1+k_B*(energydeposited/steplength) ) * steplength;
                }
    }
    else{
    	saturatedenergydeposited = 0.;
    }
	  
    fEventAction->AddScin(energydeposited); //All energy deposited in scin fibers (not saturated)
  	G4double copynumbertower = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(2); 
    G4double copynumberslice = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(3); 
	G4int Sfibercopynumber = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(1);
	  
	std::string LengthFibr =  step->GetPreStepPoint()->GetTouchableHandle()->GetVolume(1)->GetName(); 
	  
	G4double S_fiber_ID = 0;
	
    if (copynumbertower > 0){ //im in barrel right or endcap right
     fEventAction->AddVectorScinEnergyR(saturatedenergydeposited,copynumbertower, copynumberslice); //energy deposited in any scintillating fiber (saturated)
	 fEventAction->AddVectorR(energydeposited, copynumbertower, copynumberslice);
	 //I want unique Fiber ID: 168750000 is the max of Sfibercopynumber
	 S_fiber_ID = Sfibercopynumber+(168750000*copynumberslice);
	}  
	  
  	if (copynumbertower < 0){ //im in barrel left or endcap left
  	 fEventAction->AddVectorScinEnergyL(saturatedenergydeposited, copynumbertower, copynumberslice);
  	 fEventAction->AddVectorL(energydeposited, copynumbertower, copynumberslice);
	 //I want unique Fiber ID: 168750000 is the max of Sfibercopynumber
	 S_fiber_ID = Sfibercopynumber-(168750000*copynumberslice);
	}
		
	// Fibers routine: fill the S fibres info 
	if (saturatedenergydeposited>0.){
	G4VPhysicalVolume* physVol=step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
		
	//G4ThreeVector vectPos(0,0,0);
	G4int k=0;
		
		//LOCAL TO GLOBAL TRANSFORMATIONS
		G4TouchableHandle theTouchable = step->GetPreStepPoint()->GetTouchableHandle();
  		G4ThreeVector origin(0.,0.,0.);
		G4ThreeVector zdir(0.,0.,1.);
  		G4ThreeVector vectPos = theTouchable->GetHistory()->
    	GetTopTransform().Inverse().TransformPoint(origin);
		G4ThreeVector direction = theTouchable->GetHistory()->
    	GetTopTransform().Inverse().TransformAxis(zdir);
		G4double lengthfiber = atof(LengthFibr.c_str());
		G4ThreeVector Halffibervect = direction*lengthfiber/2;
		// Fibre tip position
		G4ThreeVector vectPostip = vectPos-Halffibervect;
		// SiPM position
		G4ThreeVector SiPMvecPos = vectPos+Halffibervect;
		
		fEventAction->WriteFiber_Info(S_fiber_ID,saturatedenergydeposited,1,vectPostip,copynumberslice,copynumbertower);// 1 == S 0 == C
		
		// Extract info for z time
		std::ofstream TimeFile;
		TimeFile.open("Time.txt", std::ios_base::app);
	  	TimeFile<<"Scin "<< std::fixed << std::setprecision(8) <<S_fiber_ID<<" "<<12.5*saturatedenergydeposited<<" "<<sqrt((SiPMvecPos[0]-step->GetTrack()->GetPosition().getX())*(SiPMvecPos[0]-step->GetTrack()->GetPosition().getX())+(SiPMvecPos[1]-step->GetTrack()->GetPosition().getY())*(SiPMvecPos[1]-step->GetTrack()->GetPosition().getY())+(SiPMvecPos[2]-step->GetTrack()->GetPosition().getZ())*(SiPMvecPos[2]-step->GetTrack()->GetPosition().getZ()))<<" "<<step->GetTrack()->GetGlobalTime()<<G4endl;
		TimeFile.close();
	}
  }

  if ( strstr(Fiber.c_str(),C_fiber.c_str())){//it's a Cherenkov fiber
    //Function to add up energy deposited in Cherenkov fibres
	fEventAction->AddCher(step->GetTotalEnergyDeposit());
  }
//part for cherenkov photons
G4OpBoundaryProcessStatus theStatus = Undefined;

G4ProcessManager* OpManager =
                     G4OpticalPhoton::OpticalPhoton()->GetProcessManager();

 if (OpManager) {
     G4int MAXofPostStepLoops =
              OpManager->GetPostStepProcessVector()->entries();
     G4ProcessVector* fPostStepDoItVector =
              OpManager->GetPostStepProcessVector(typeDoIt);

     for ( G4int i=0; i<MAXofPostStepLoops; i++) {
         G4VProcess* fCurrentProcess = (*fPostStepDoItVector)[i];
         fOpProcess = dynamic_cast<G4OpBoundaryProcess*>(fCurrentProcess);
         if (fOpProcess) { theStatus = fOpProcess->GetStatus(); break;}
     }
  }

  std::string SiPMC = "SiPMC";
  std::string SiPMS = "SiPMS";
  std::string SiPMdetection;

  //If the particle is an optical photon...
  if(step->GetTrack()->GetDefinition()->GetParticleName() == "opticalphoton"){

     switch (theStatus){

        case TotalInternalReflection: 
           Fiber = PreStepVolume->GetName();

            if(strstr(Fiber.c_str(), C_fiber.c_str())){ //it's a Cherenkov fibre
				G4double copynumbertower = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(2); 
				G4double copynumberslice = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(3); 
				G4int Cfibercopynumber = step->GetPreStepPoint()->GetTouchableHandle()->GetCopyNumber(1);

				std::string LengthFibr =  step->GetPreStepPoint()->GetTouchableHandle()->GetVolume(1)->GetName(); 
				
				G4double C_fiber_ID = 0;
			
    		   if (copynumbertower>0){ //i'm in barrel right or endcap right
	    		   fEventAction->AddVectorCherPER(copynumbertower, copynumberslice);
			   	 //I want unique Fiber ID: 168750000 is the max of Cfibercopynumber
	 			 C_fiber_ID = Cfibercopynumber+(168750000*copynumberslice);
			   }
	    	   if (copynumbertower<0){ //i'm in barrel left ot endcap left
	    	   	   fEventAction->AddVectorCherPEL(copynumbertower, copynumberslice);
			   //I want unique Fiber ID: 168750000 is the max of Cfibercopynumber
	 			 C_fiber_ID = Cfibercopynumber-(168750000*copynumberslice);
			   }
	    	   fEventAction->AddCherenkov(); // add one photoelectron from Cherenkov process in Cherenkov fibers                  
				
				// Fibers routine: fill the C fibres info 
				G4VPhysicalVolume* physVol=step->GetPreStepPoint()->GetTouchableHandle()->GetVolume();
		
				G4int k=0;
				G4TouchableHandle theTouchable = step->GetPreStepPoint()->GetTouchableHandle();
  				G4ThreeVector origin(0.,0.,0.);
				G4ThreeVector zdir(0.,0.,1.);
  				G4ThreeVector vectPos = theTouchable->GetHistory()->
    			GetTopTransform().Inverse().TransformPoint(origin);
				G4ThreeVector direction = theTouchable->GetHistory()->
    			GetTopTransform().Inverse().TransformAxis(zdir);
				G4double lengthfiber = atof(LengthFibr.c_str());
				G4ThreeVector Halffibervect = direction*lengthfiber/2;
				// Fibre tip position
				G4ThreeVector vectPostip = vectPos-Halffibervect;
				// SiPM position
				G4ThreeVector SiPMvecPos = vectPos+Halffibervect;
	
				fEventAction->WriteFiber_Info(C_fiber_ID,1,0,vectPostip,copynumberslice,copynumbertower);// 1 == S 0 == C
				
				step->GetTrack()->SetTrackStatus(fStopAndKill); //kill photon
				// Extract info for z time
				std::ofstream TimeFile;
				TimeFile.open("Time.txt", std::ios_base::app);
	  			TimeFile<<"Cher "<<std::fixed << std::setprecision(8) <<C_fiber_ID<<" "<<1<<" "<<sqrt((SiPMvecPos[0]-step->GetTrack()->GetPosition().getX())*(SiPMvecPos[0]-step->GetTrack()->GetPosition().getX())+(SiPMvecPos[1]-step->GetTrack()->GetPosition().getY())*(SiPMvecPos[1]-step->GetTrack()->GetPosition().getY())+(SiPMvecPos[2]-step->GetTrack()->GetPosition().getZ())*(SiPMvecPos[2]-step->GetTrack()->GetPosition().getZ()))<<" "<<step->GetTrack()->GetGlobalTime()<<G4endl;
				TimeFile.close();
				
           	}
           	break;
               /*
               Prestep = step->GetPreStepPoint()->GetPosition();   
               Postsep = step->GetPostStepPoint()->GetPosition();
               Momentum = step->GetTrack()->GetMomentumDirection();
               if(Momentum.z()>0.){ //the photon is going towards SiPms
                costheta = Momentum.z();
                if(costheta>0.99){//0.94*/ //if the photon is under the acceptance angle of fibers
                  /* only if you want exponential light attenuation
                  distance = (1560.9-Prestep.z())/costheta;
                  pSurvive = std::exp(-(distance/8900));
                  pTot=PSurvive*pDetection;*/
/*                  pTot =pDetection;
                  if(pRandom<pTot){  
                    fEventAction->AddCherenkov(); // add one photoelectron from Cherenkov process in Cherenkov fibers                  
                    //fEventAction->AddSignalfibre(copynumber); //only if you want SignalFibre
                    fEventAction->AddVectorCherPE(copynumbermodule,copynumber);
                    step->GetTrack()->SetTrackStatus(fStopAndKill); //I kille the photon just after having counted it or excluded
/*                  }
                }
              }
             }
    break;

  case Detection:
  // if you want no parameterization and complete full simulation uncomment this part
    
   /* SiPMdetection = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetName();
    if (strstr(SiPMdetection.c_str(),SiPMC.c_str()))
     {
       fEventAction->AddCherenkov();
     } 
   
    if (strstr(SiPMdetection.c_str(),SiPMS.c_str()))
    {
      fEventAction->AddScintillation();
    }
 
  break;*/

  default: 
     //only for parameterization, comment for full simulation
     step->GetTrack()->SetTrackStatus(fStopAndKill);
  break;
  }
}


}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
