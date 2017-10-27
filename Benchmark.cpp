//
//  Benchmark.cpp
//  VanAllen
//
//  Created by Gabriele Gaetano Fronzé on 09/03/15.
//  Copyright (c) 2015 Gabriele Gaetano Fronzé. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <TROOT.h>
#include <TFile.h>
#include <TH2D.h>
#include <TStopwatch.h>

int Benchmark(int nelectrons=800){

    FILE *fp;
    fp = fopen("timerlog.out", "w");
    int mpiProcessesNumber[6]={3,4,6,10,14,18};
    int multiplier[6]={1,2,5,10,20,100};
    int nRuns=3;
    double times[6][6];
    double seqtime;
    TH2D *histo=new TH2D(Form("Benchmark %d electrons",nelectrons),Form("Benchmark %d electrons",nelectrons),6,.5,6.5,6,0.5,6.5);
    TStopwatch stoppy;

    for(int runc=0;runc<nRuns;runc++){
        char command[500];
        sprintf(command,"xterm -e ./VanAllenSeq %d",nelectrons);
        printf("%s\n", command);
        stoppy.Reset();
        stoppy.Start();
        system(command);
        stoppy.Stop();
        std::cout<<stoppy.RealTime()<<std::endl;
        seqtime+=stoppy.RealTime();
    }
    seqtime/=nRuns;
    
    for(int processc=0;processc<6;processc++){
        for(int taskc=0;taskc<6;taskc++){
            for(int runc=0;runc<nRuns;runc++){
                char command[500];
                sprintf(command,"xterm -e mpiexec -np %d ./VanAllen 0 %d %d 0",mpiProcessesNumber[processc],nelectrons,multiplier[taskc]);
                printf("%s\n", command);
                stoppy.Reset();
                stoppy.Start();
                system(command);
                stoppy.Stop();
                std::cout<<stoppy.RealTime()<<std::endl;
                times[taskc][processc]+=stoppy.RealTime();
            }
            times[taskc][processc]/=nRuns;
            std::cout<<times[taskc][processc]<<std::endl;
            histo->SetBinContent(taskc, processc, seqtime/times[taskc][processc]);
        }
    }
    
    histo->Draw("LEGOZ");
    
//    TFile *file=new TFile("benchmark.root","UPDATE");
//    file->cd();
//    histo->SetDirectory(0);
//    histo->Write();
//    file->Close();
    
    return 0;
}