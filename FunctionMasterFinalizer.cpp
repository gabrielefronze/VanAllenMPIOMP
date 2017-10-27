//
//  FunctionMaster.cpp
//  VanAllen
//
//  Created by Gabriele Gaetano Fronzé on 05/03/15.
//  Copyright (c) 2015 Gabriele Gaetano Fronzé. All rights reserved.
//

#ifndef define
#include "Tags.h"
#include "Params.h"
#include "socketcomm.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include "mpi.h"
#include <omp.h>
#include <TROOT.h>
#include <TRandom2.h>
#include <TStopwatch.h>
#include <TH1D.h>
#include <TPaveStats.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TList.h>
#include <unistd.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

using namespace std;

void FunctionMasterFinalizer(int Size,int NelMin,int NelMax,int TaskMultiplier,int Master,int Finalizer,int WorkerNumber,int wFinalizer,MPI_Comm gathercomm,int monitor,unsigned int Seed){
    
    int socketID;
    int Nel=NelMax-NelMin;
    if(monitor==1){
        int portno;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        portno = 51717;
        socketID = socket(AF_INET, SOCK_STREAM, 0);
        char hostname[9]={'l','o','c','a','l','h','o','s','t'};
        server = gethostbyname(hostname);
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
        serv_addr.sin_port = htons(portno);
        connect(socketID,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
        
        socketwrite(Welcome,socketID);
        socketsend(Master, socketID);
        socketsend(Finalizer, socketID);
        socketsend(Size, socketID);
        socketsend((NelMax-NelMin), socketID);
        socketsend(TaskMultiplier, socketID);
    }else{
        printf("\n"
               "\n"
               "   ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
               "   +                                                                          +\n"
               "   +                             WELCOME TO VANALLEN                          +\n"
               "   +               a software developed by G.G. Fronzé and M. Mina            +\n"
               "   +                                                                          +\n"
               "   ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
               "   +                                                                          +\n"
               "   + This program will simulate the electron drift mote crossing Earth        +\n"
               "   + magnetic field.                                                          +\n"
               "   +                                                                          +\n"
               "   ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
               "   + The simulation is running on %3d machines or workers, each with possibly +\n"
               "   + a different kind of CPU. This program tries to use the full power of the +\n"
               "   + machines connected, assigning jobs on demand customized on the local     +\n"
               "   + computational power.                                                     +\n"
               "   ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
               "   +                                                                          +\n"
               "   + For this run Master and Finalizer are on ranks %3d and %3d.              +\n"
               "   +                                                                          +\n"
               "   + This run can be exactly reproduced giving it the same RNG seed. We use a +\n"
               "   + PCG64 RNG capable of 2^127 different sequences with 2^128 period.        +\n"
               "   +                                                                          +\n"
               "   + Output file is produced using the CERN ROOT Framework.                   +\n"
               "   + To open it please use a TBrowser instance.                               +\n"
               "   +                                                                          +\n"
               "   +    Have sincerely fun                                                    +\n"
               "   +                                                                          +\n"
               "   ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n",Size,Master,Finalizer);
    }
    
    /*Time 0 per l'Uptime del processo*/
//    TStopwatch stoppy=TStopwatch();
//    stoppy.Start();
    
    /*Broadcast del seed*/
    MPI_Bcast(&wFinalizer,1,MPI_INT,Master,MPI_COMM_WORLD);
    MPI_Bcast(&Seed, 1, MPI_INT, Master, MPI_COMM_WORLD);
    
    
    /*Variabili MPI per l'handling delle comunicazioni*/
    MPI_Status status;
    MPI_Request req;
    double checksum[2]={0.,0.};
    int dummy;
    double dummyd[2];
    int tasknumberbuffer;
    int *threadNumber= new int[Size];
    int *taskCount=new int[Size];
    double *realTimes=new double[Size];
    double *CPUTimes=new double[Size];
    
    
    

    
#pragma omp parallel for
    for(int i=0;i<Size;i++){
        threadNumber[i]=0;
        taskCount[i]=0;
    }
    
    for(int i=0;i<WorkerNumber;i++){
        int buffer=0;
        MPI_Recv(&buffer,1,MPI_INT,MPI_ANY_SOURCE,NProcess,MPI_COMM_WORLD,&status);
        threadNumber[status.MPI_SOURCE]=buffer*TaskMultiplier;
    }
    
    /*Inizializzatione ed avvio dei worker, sottomissione dei job*/
    int totTaskCount=0;
    int elCount=NelMin;
    
    if(monitor==1){
        socketwrite(Monitor,socketID);
        socketsend(threadNumber,Size,socketID);
#pragma omp parallel sections num_threads(2)
        {
#pragma omp section
            {
                double Time[2];
                while(elCount<NelMax){
                    /*Avvio della comunicazione a 3 step, TAG TaskQuery*/
                    MPI_Recv(Time,2,MPI_DOUBLE,MPI_ANY_SOURCE,TaskQuery,MPI_COMM_WORLD,&status);
                    MPI_Ssend(&elCount,1,MPI_INT,status.MPI_SOURCE,GoOn,MPI_COMM_WORLD);
                    elCount+=threadNumber[status.MPI_SOURCE];
                    taskCount[status.MPI_SOURCE]++;
                    realTimes[status.MPI_SOURCE]=Time[0];
                    CPUTimes[status.MPI_SOURCE]=Time[1];
                    totTaskCount++;
                    checksum[0]+=Time[0];
                }
                
                /*I worker vengono fermati attendendo richieste di lavoro ulteriori e comunicando il TAG di arresto*/
                for(int l=0;l<WorkerNumber;l++){
                    MPI_Recv(&dummyd,2,MPI_DOUBLE,MPI_ANY_SOURCE,TaskQuery,MPI_COMM_WORLD,&status);
                    MPI_Ssend(&dummy,1,MPI_INT,status.MPI_SOURCE,Stop,MPI_COMM_WORLD);
                }
            }
            
#pragma omp section
            {
                while(elCount<NelMax){
                    while(checksum[0]==checksum[1]){
                        usleep(10000);
                    }
                    checksum[1]=checksum[0];
                    socketsend(elCount, socketID);
                    socketsend(taskCount, Size, socketID);
                    socketsend(realTimes, Size, socketID);
                    socketsend(CPUTimes, Size, socketID);
                }
                socketsend(-1, socketID);
                socketsend(taskCount, Size, socketID);
                socketsend(realTimes, Size, socketID);
                socketsend(CPUTimes, Size, socketID);
            }
        }
    }else{
        double Time[2];
        while(elCount<NelMax){
            /*Avvio della comunicazione a 3 step, TAG TaskQuery*/
            MPI_Recv(Time,2,MPI_DOUBLE,MPI_ANY_SOURCE,TaskQuery,MPI_COMM_WORLD,&status);
            MPI_Ssend(&elCount,1,MPI_INT,status.MPI_SOURCE,GoOn,MPI_COMM_WORLD);
            elCount+=threadNumber[status.MPI_SOURCE];
            totTaskCount++;
        }
        
        /*I worker vengono fermati attendendo richieste di lavoro ulteriori e comunicando il TAG di arresto*/
        for(int l=0;l<WorkerNumber;l++){
            MPI_Recv(&dummyd,2,MPI_DOUBLE,MPI_ANY_SOURCE,TaskQuery,MPI_COMM_WORLD,&status);
            MPI_Ssend(&dummy,1,MPI_INT,status.MPI_SOURCE,Stop,MPI_COMM_WORLD);
            cout<<l<<endl;
        }
    }
    
    delete[] threadNumber;
    delete[] taskCount;
    delete[] realTimes;
    delete[] CPUTimes;
    
    if(monitor==1){
        MPI_Recv(&dummy,1,MPI_INT,MPI_ANY_SOURCE,Mean,MPI_COMM_WORLD,&status);
        socketwrite(MeanBeg,socketID);
        MPI_Recv(&dummy,1,MPI_INT,MPI_ANY_SOURCE,Mean,MPI_COMM_WORLD,&status);
        socketwrite(MeanBeg,socketID);
    }
    
    /*Variabili per l'handling delle comunicazioni*/
    int totalel=0;
    
    /*Variabili di buffer e di conteggio*/
    int threadnumber=omp_get_num_procs();
    
    vector<double> alldevst(steps);
    vector<double> devst(steps);
    fill(devst.begin(),devst.end(),0.0);
    fill(alldevst.begin(),alldevst.end(),0.0);
    
    socketwrite(DevBeg,socketID);
    MPI_Reduce(&devst[0],&alldevst[0],steps,MPI_DOUBLE,MPI_SUM,wFinalizer,gathercomm);
    socketwrite(DevBeg,socketID);
    
    TFile *file=new TFile(Form("VanAllenOut-Nel={%d,%d}-tm=x%d-w=%d.root",NelMin,NelMax,TaskMultiplier,Size),"UPDATE");
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
    
    socketwrite(HistBeg,socketID);
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
    socketwrite(HistBeg,socketID);
    
    histo->Merge(histolist);
    delete histolist;
    delete[] displ;
    
    socketwrite(File,socketID);
    file->cd();
    histo->GetXaxis()->SetTitle("time [s]");
    histo->GetYaxis()->SetTitle("variance [srad^2]");
    histo->Write();
    file->Close();
    socketwrite(File,socketID);
    
    /*Deallocazione*/
    devst.clear();
    devst.shrink_to_fit();
    alldevst.clear();
    alldevst.shrink_to_fit();

    if(monitor==1) closesocket(socketID);
    
    return;
}