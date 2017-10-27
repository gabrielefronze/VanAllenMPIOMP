//
//  FunctionFinalizer.cpp
//  VanAllen
//
//  Created by Gabriele Gaetano Fronzé on 05/03/15.
//  Copyright (c) 2015 Gabriele Gaetano Fronzé. All rights reserved.
//

#ifndef define
#include "Functions.h"
#include "Tags.h"
#include "Params.h"
#include <omp.h>
#include <stdio.h>
#include <iostream>
#include <vector>
//#include <algorithm>
//#include <numeric>
#include "mpi.h"
#include <TROOT.h>
#include <TStopwatch.h>
#include <TH1D.h>
#include <TPaveStats.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TList.h>
//#include <ncurses.h>
#endif

using namespace std;

void FunctionFinalizer(int size,int NelMin,int NelMax,int TaskMultiplier,int Master,int Finalizer,int WorkerNumber,int wFinalizer,MPI_Comm gathercomm){
    /*Time 0 per l'Uptime del processo*/
//    TStopwatch stoppy=TStopwatch();
//    stoppy.Start();
    
    /*Variabili per l'handling delle comunicazioni*/
    MPI_Status status;
    MPI_Request req;
    int dummy=0;
    int totalel=0;
    
    MPI_Bcast(&wFinalizer,1,MPI_INT,Finalizer,MPI_COMM_WORLD);
    MPI_Bcast(&dummy,1,MPI_INT,Master,MPI_COMM_WORLD);
    
    /*Variabili di buffer e di conteggio*/
    int threadnumber=omp_get_num_procs();
    
    vector<double> alldevst(steps);
    vector<double> devst(steps);
    fill(devst.begin(),devst.end(),0.0);
    fill(alldevst.begin(),alldevst.end(),0.0);
    
    MPI_Reduce(&devst[0],&alldevst[0],steps,MPI_DOUBLE,MPI_SUM,wFinalizer,gathercomm);
    
    TFile *file=new TFile(Form("VanAllenOut-Nel={%d,%d}-tm=x%d-w=%d.root",NelMin,NelMax,TaskMultiplier,size),"UPDATE");
    TH1D *histo=new TH1D("histo",Form("Istogramma {%d,%d} con %d dati",NelMin,NelMax,steps),steps,0,steps*dt*wce/(2*pi));
    vector<TH1D*> histovect;
    TList *histolist=new TList;
    int *displ=new int [threadnumber+1];
    
    for(int i=0;i<threadnumber;i++){
        histovect.push_back(new TH1D(Form("histo%d",i),Form("histo%d",i),histo->GetXaxis()->GetNbins(),histo->GetXaxis()->GetXmin(),histo->GetXaxis()->GetXmax()));
    }
    
    displ[0]=0;
    for(int i=1;i<threadnumber;i++){
        displ[i]=steps/threadnumber+displ[i-1];
    }
    displ[threadnumber]=steps;
    
    MPI_Isend(&dummy, 1, MPI_INT, Master, Histo, MPI_COMM_WORLD, &req);
    MPI_Request_free(&req);
#pragma omp parallel num_threads(threadnumber) shared(histolist)
    {
        int threadRank=omp_get_thread_num();
        
        for(int p=displ[threadRank];p<displ[threadRank+1];p++){
            histovect[threadRank]->SetBinContent(p+1,alldevst[p]/(double)(NelMax-NelMin));
        }
        
#pragma omp critical
        {
            histolist->AddAt((TH1D*)(histovect[threadRank]->Clone(Form("h%d",threadRank))),threadRank);
        }
        
        delete histovect[threadRank];
        
#pragma omp barrier
    }
    MPI_Isend(&dummy, 1, MPI_INT, Master, Histo, MPI_COMM_WORLD, &req);
    MPI_Request_free(&req);
    
    histo->Merge(histolist);
    delete histolist;
    delete[] displ;

    MPI_Isend(&dummy, 1, MPI_INT, Master, File, MPI_COMM_WORLD, &req);
    MPI_Request_free(&req);
    file->cd();
    histo->GetXaxis()->SetTitle("time [s]");
    histo->GetYaxis()->SetTitle("variance [srad^2]");
    histo->Write();
    file->Close();
    MPI_Isend(&dummy, 1, MPI_INT, Master, File, MPI_COMM_WORLD, &req);
    MPI_Request_free(&req);
    
    /*Deallocazione*/
    devst.clear();
    devst.shrink_to_fit();
    alldevst.clear();
    alldevst.shrink_to_fit();
    
    //stoppy.Stop();

    return;
}
