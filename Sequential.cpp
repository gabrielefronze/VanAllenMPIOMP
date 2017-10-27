//
//  Sequential.cpp
//  VanAllenMonitor
//
//  Created by Gabriele Gaetano Fronzé on 01/04/15.
//  Copyright (c) 2015 Gabriele Gaetano Fronzé. All rights reserved.
//

#include <stdio.h>
#include "Tags.h"
#include "Params.h"
#include "DiffFuncVector.h"
// #include <omp.h>
#include <stdio.h>
#include <vector>
#include "mpi.h"
#include <TROOT.h>
#include <TStopwatch.h>
#include <unistd.h>
#include "Tags.h"
#include "Params.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <TROOT.h>
#include <TStopwatch.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TList.h>

using namespace std;

int main(int argc, char* argv[]){

    TStopwatch stoppy;
    stoppy.Start();
    vector<double> resarr;
    int Nel=40000;
    if(argc>=2)Nel=atoi(argv[1]);

    for(int i=0;i<Nel;i++){
        DiffusionFunctionVector(resarr, i,1234);
    }

    vector<double> localsum(steps);
    vector<double> devst(steps);
    int totalel=0;

    /*L'indipendenza dell'output permette lo svolgimento in parallelo degli accumuli*/
    for(int st=0;st<steps;st++){
        for(int el=0;el<Nel;el++){
            localsum[st]+=resarr[el*steps+st];
        }
    }

    /*Come sopra*/
    for(int st=0;st<steps;st++){
        double mean=localsum[st]/(double)Nel;
        for(int el=0;el<Nel;el++){
            devst[st]+=(resarr[el*steps+st]-mean)*(resarr[el*steps+st]-mean);
        }
    }

    TFile *file=new TFile(Form("VanAllenOutSeq-Nel=%d.root",Nel),"UPDATE");
    TH1D *histo=new TH1D("histo",Form("Istogramma %d con %d dati",Nel,steps),steps,0,steps*dt*wce/(2*pi));

    for(int p=0;p<steps;p++){
        histo->SetBinContent(p,devst[p]/(double)Nel);
    }

    file->cd();
    histo->Write();
    file->Close();

    //delete histo;

    stoppy.Stop();
    stoppy.Print();

}
